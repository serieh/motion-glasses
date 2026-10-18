"""Unit test suite for Motion Glasses navigation logic and state machine."""

import pytest

from motion_glasses.controller import (
    MotionGlassesController,
    NavigationMode,
    SensorReadings,
)


@pytest.fixture
def controller() -> MotionGlassesController:
    c = MotionGlassesController(distance_threshold=100.0, cycle_limit=46, down_angle=30.0)
    c.set_level([120.0] * 10)
    return c


def test_set_level_calibration():
    c = MotionGlassesController()
    samples = [110.0, 115.0, 120.0, 125.0, 110.0, 115.0, 120.0, 125.0, 110.0, 115.0]
    avg = sum(samples) / 10.0
    res = c.set_level(samples)
    assert res == pytest.approx(avg)
    assert c.level == pytest.approx(avg)

    with pytest.raises(ValueError):
        c.set_level([])


def test_pitch_from_accel():
    c = MotionGlassesController()
    assert c.pitch_from_accel(0.0) == pytest.approx(0.0)
    assert c.pitch_from_accel(1.0) == pytest.approx(-90.0)
    assert c.pitch_from_accel(-1.0) == pytest.approx(90.0)


def test_sleep_mode_when_sensor_zero(controller: MotionGlassesController):
    # Front is 0
    mode, vib = controller.step(SensorReadings(df=0.0, dr=120.0, dl=120.0, pitch=45.0))
    assert mode == NavigationMode.SLEEP
    assert not vib.active

    # Right is 0
    mode, vib = controller.step(SensorReadings(df=150.0, dr=0.0, dl=120.0, pitch=45.0))
    assert mode == NavigationMode.SLEEP
    assert not vib.active

    # Left is 0
    mode, vib = controller.step(SensorReadings(df=150.0, dr=120.0, dl=0.0, pitch=45.0))
    assert mode == NavigationMode.SLEEP
    assert not vib.active


def test_straight_mode_clear_path(controller: MotionGlassesController):
    mode, vib = controller.step(SensorReadings(df=150.0, dr=150.0, dl=150.0, pitch=45.0))
    assert mode == NavigationMode.STRAIGHT
    assert not vib.active


def test_straight_mode_front_obstacle(controller: MotionGlassesController):
    mode, vib = controller.step(SensorReadings(df=80.0, dr=150.0, dl=150.0, pitch=45.0))
    assert mode == NavigationMode.STRAIGHT
    assert vib.both


def test_straight_mode_right_obstacle(controller: MotionGlassesController):
    mode, vib = controller.step(SensorReadings(df=150.0, dr=70.0, dl=150.0, pitch=45.0))
    assert mode == NavigationMode.STRAIGHT
    assert vib.right and not vib.left


def test_straight_mode_left_obstacle(controller: MotionGlassesController):
    mode, vib = controller.step(SensorReadings(df=150.0, dr=150.0, dl=70.0, pitch=45.0))
    assert mode == NavigationMode.STRAIGHT
    assert vib.left and not vib.right


def test_straight_mode_front_priority_over_sides(controller: MotionGlassesController):
    # Front, right, left all below threshold -> front takes precedence and vibrates both
    mode, vib = controller.step(SensorReadings(df=80.0, dr=60.0, dl=60.0, pitch=45.0))
    assert mode == NavigationMode.STRAIGHT
    assert vib.both


def test_down_mode_curb_obstacle(controller: MotionGlassesController):
    # Pitch is 20 deg (< 30 deg), df=80cm (<= level 120cm)
    mode, vib = controller.step(SensorReadings(df=80.0, dr=100.0, dl=100.0, pitch=20.0))
    assert mode == NavigationMode.DOWN
    assert vib.right and not vib.left


def test_down_mode_dropoff_hole(controller: MotionGlassesController):
    # Pitch is 20 deg (< 30 deg), df=160cm (> level 120cm)
    mode, vib = controller.step(SensorReadings(df=160.0, dr=100.0, dl=100.0, pitch=20.0))
    assert mode == NavigationMode.DOWN
    assert vib.left and not vib.right


def test_cycle_limit_auto_mute(controller: MotionGlassesController):
    # Step 1: obstacle appears, vibration activates
    reading = SensorReadings(df=60.0, dr=150.0, dl=150.0, pitch=45.0)
    mode, vib = controller.step(reading)
    assert vib.both

    # Send 46 more identical readings (total 47 calls)
    for _ in range(46):
        mode, vib = controller.step(reading)
        assert vib.both

    # At call 48, cycle_f is >= 46 when straight_mode evaluates, suppressing vibration
    mode, vib = controller.step(reading)
    assert not vib.active

    # When distance changes, cycle counter resets to 0 at the end of the loop
    new_reading = SensorReadings(df=59.0, dr=150.0, dl=150.0, pitch=45.0)
    controller.step(new_reading)
    assert controller.cycle_f == 0

    # On the next cycle, vibration re-activates with cycle_f == 0
    mode, vib = controller.step(new_reading)
    assert vib.both


def test_no_cycle_switch_prevents_mute(controller: MotionGlassesController):
    reading = SensorReadings(df=60.0, dr=150.0, dl=150.0, pitch=45.0)
    for _ in range(60):
        mode, vib = controller.step(reading, no_cycle_switch=True)
        assert controller.cycle_f == 0
        assert vib.both


def test_straight_mode_switch_override(controller: MotionGlassesController):
    # Head is tilted down (pitch 15 deg), but str_switch is ON
    reading = SensorReadings(df=60.0, dr=150.0, dl=150.0, pitch=15.0)
    mode, vib = controller.step(reading, str_switch=True)
    assert mode == NavigationMode.STRAIGHT
    assert vib.both
