"""Firmware logic controller and navigation state machine."""

from __future__ import annotations

import math
from dataclasses import dataclass
from enum import Enum


class NavigationMode(str, Enum):
    """Operational mode of the navigation glasses."""

    SLEEP = "SLEEP"
    STRAIGHT = "STRAIGHT"
    DOWN = "DOWN"


@dataclass
class VibrationState:
    """Status of left and right haptic vibration motors."""

    left: bool = False
    right: bool = False

    @property
    def both(self) -> bool:
        return self.left and self.right

    @property
    def active(self) -> bool:
        return self.left or self.right

    def __str__(self) -> str:
        if self.both:
            return "BOTH (FRONT OBSTACLE)"
        if self.left:
            return "LEFT MOTOR ON"
        if self.right:
            return "RIGHT MOTOR ON"
        return "OFF"


@dataclass
class SensorReadings:
    """Sensor readings from 3 ultrasonic sensors and pitch angle."""

    df: float  # Front distance in cm
    dr: float  # Right distance in cm
    dl: float  # Left distance in cm
    pitch: float = 45.0  # Head pitch angle in degrees (positive = forward/up)


class MotionGlassesController:
    """State machine replicating the Motion Glasses firmware logic."""

    def __init__(
        self,
        distance_threshold: float = 100.0,
        cycle_limit: int = 46,
        down_angle: float = 30.0,
    ) -> None:
        self.distance_threshold = distance_threshold
        self.cycle_limit = cycle_limit
        self.down_angle = down_angle

        # Calibrated ground surface distance in cm
        self.level: float = 100.0

        # Cycle counters
        self.cycle_f: int = 0
        self.cycle_r: int = 0
        self.cycle_l: int = 0

        # Previous distance values
        self.pre_df: float = 0.0
        self.pre_dr: float = 0.0
        self.pre_dl: float = 0.0

        # Current state
        self.vibrators = VibrationState()
        self.current_mode = NavigationMode.SLEEP

    def set_level(self, calibration_samples: list[float]) -> float:
        """Calibrate the user's standing surface ground level distance."""
        if not calibration_samples:
            raise ValueError("Calibration samples cannot be empty")
        self.level = sum(calibration_samples) / float(len(calibration_samples))
        return self.level

    @staticmethod
    def pitch_from_accel(x_axis: float) -> float:
        """Calculate pitch angle in degrees from normalized X-axis acceleration."""
        clamped_x = max(-1.0, min(1.0, x_axis))
        return -1.0 * math.asin(clamped_x) * 180.0 / math.pi

    def straight_mode(self, df: float, dr: float, dl: float) -> VibrationState:
        """Handle straight navigation obstacle avoidance."""
        v_left = False
        v_right = False

        if df <= self.distance_threshold and self.cycle_f < self.cycle_limit:
            v_right = True
            v_left = True
        else:
            if dr <= self.distance_threshold and self.cycle_r < self.cycle_limit:
                v_right = True
            if dl <= self.distance_threshold and self.cycle_l < self.cycle_limit:
                v_left = True

        return VibrationState(left=v_left, right=v_right)

    def down_mode(self, df: float) -> VibrationState:
        """Handle ground surface tracking and drop-off / curb detection."""
        v_left = False
        v_right = False

        if self.cycle_f < self.cycle_limit:
            if df <= self.level:
                # Obstacle / raised surface / step up -> right motor
                v_right = True
            else:
                # Drop-off / hole / step down -> left motor
                v_left = True

        return VibrationState(left=v_left, right=v_right)

    def update_cycle_count(self, df: float, dr: float, dl: float) -> None:
        """Increment stationary cycle counters when consecutive readings match."""
        self.cycle_f = (self.cycle_f + 1) if self.pre_df == df else 0
        self.cycle_r = (self.cycle_r + 1) if self.pre_dr == dr else 0
        self.cycle_l = (self.cycle_l + 1) if self.pre_dl == dl else 0

    def step(
        self,
        readings: SensorReadings,
        str_switch: bool = False,
        no_cycle_switch: bool = False,
    ) -> tuple[NavigationMode, VibrationState]:
        """Process one loop cycle of sensor readings and return mode & vibration output."""
        df = readings.df
        dr = readings.dr
        dl = readings.dl
        pitch = readings.pitch

        # Check for sensor sleep / fault (any sensor reading 0)
        if df == 0 or dr == 0 or dl == 0:
            self.current_mode = NavigationMode.SLEEP
            self.vibrators = VibrationState(left=False, right=False)
            return self.current_mode, self.vibrators

        # Mode determination: Pitch > down_angle OR straight-mode switch ON
        if pitch > self.down_angle or str_switch:
            self.current_mode = NavigationMode.STRAIGHT
            self.vibrators = self.straight_mode(df, dr, dl)
        else:
            self.current_mode = NavigationMode.DOWN
            self.vibrators = self.down_mode(df)

        # Update or reset cycle counters based on no_cycle_switch
        if not no_cycle_switch:
            self.update_cycle_count(df, dr, dl)
        else:
            self.cycle_f = 0
            self.cycle_r = 0
            self.cycle_l = 0

        # Store current readings as previous readings for next cycle
        self.pre_df = df
        self.pre_dr = dr
        self.pre_dl = dl

        return self.current_mode, self.vibrators
