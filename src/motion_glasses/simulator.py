"""Scenario simulator and verification test bench for Motion Glasses."""

from __future__ import annotations

from typing import NamedTuple

from motion_glasses.controller import (
    MotionGlassesController,
    NavigationMode,
    SensorReadings,
    VibrationState,
)


class StepLog(NamedTuple):
    cycle: int
    pitch: float
    df: float
    dr: float
    dl: float
    mode: NavigationMode
    vibration: VibrationState
    cycle_f: int
    cycle_r: int
    cycle_l: int


def run_scenario(
    name: str,
    steps: list[SensorReadings],
    calibrated_level: float = 120.0,
    str_switch: bool = False,
    no_cycle_switch: bool = False,
) -> list[StepLog]:
    """Execute a list of sensor reading steps and return execution logs."""
    controller = MotionGlassesController()
    controller.level = calibrated_level

    logs: list[StepLog] = []
    for idx, reading in enumerate(steps, start=1):
        mode, vib = controller.step(
            reading, str_switch=str_switch, no_cycle_switch=no_cycle_switch
        )
        logs.append(
            StepLog(
                cycle=idx,
                pitch=reading.pitch,
                df=reading.df,
                dr=reading.dr,
                dl=reading.dl,
                mode=mode,
                vibration=vib,
                cycle_f=controller.cycle_f,
                cycle_r=controller.cycle_r,
                cycle_l=controller.cycle_l,
            )
        )
    return logs


def get_default_scenarios() -> dict[str, list[SensorReadings]]:
    """Return realistic test scenarios for functional verification."""
    return {
        "Clear Path Navigation": [
            SensorReadings(df=200.0, dr=180.0, dl=190.0, pitch=40.0)
            for _ in range(5)
        ],
        "Front Obstacle & Cycle Limit Auto-Mute": (
            # 5 steps approaching
            [SensorReadings(df=80.0, dr=150.0, dl=150.0, pitch=40.0)]
            * 50  # Exceeds cycle limit of 46
        ),
        "Right Side Obstacle": [
            SensorReadings(df=180.0, dr=45.0, dl=150.0, pitch=40.0) for _ in range(5)
        ],
        "Left Side Obstacle": [
            SensorReadings(df=180.0, dr=150.0, dl=45.0, pitch=40.0) for _ in range(5)
        ],
        "Down Mode - Raised Step / Curb (df <= level)": [
            SensorReadings(df=70.0, dr=100.0, dl=100.0, pitch=15.0) for _ in range(5)
        ],
        "Down Mode - Hole / Drop-off (df > level)": [
            SensorReadings(df=160.0, dr=100.0, dl=100.0, pitch=15.0) for _ in range(5)
        ],
        "Sensor Fault / Sleep Mode (df = 0)": [
            SensorReadings(df=0.0, dr=120.0, dl=120.0, pitch=40.0) for _ in range(3)
        ],
    }
