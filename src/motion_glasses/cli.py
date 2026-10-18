"""CLI entry point for Motion Glasses simulation and verification."""

from __future__ import annotations

import argparse
import sys

from motion_glasses.controller import MotionGlassesController, SensorReadings
from motion_glasses.simulator import get_default_scenarios, run_scenario


def print_banner() -> None:
    print("=" * 72)
    print("  Motion Glasses Firmware Logic Simulator")
    print("  Assistive Dual-Mode Ultrasonic & IMU Navigation System")
    print("=" * 72)


def run_all_scenarios() -> int:
    """Execute all predefined scenarios and print structured verification results."""
    print_banner()
    scenarios = get_default_scenarios()

    for name, readings in scenarios.items():
        print(f"\n--- Scenario: {name} ({len(readings)} cycles) ---")
        logs = run_scenario(name, readings)

        # Print representative sample steps
        samples = [logs[0]]
        if len(logs) > 2:
            mid = len(logs) // 2
            samples.append(logs[mid])
        if len(logs) > 1:
            samples.append(logs[-1])

        print(
            f"{'Cycle':>6} | {'Pitch':>6} | {'Front(cm)':>9} | {'Right(cm)':>9} | "
            f"{'Left(cm)':>8} | {'Mode':>8} | {'Vibration':<24} | {'Cycles (F,R,L)'}"
        )
        print("-" * 92)

        for log in samples:
            cycle_str = f"({log.cycle_f},{log.cycle_r},{log.cycle_l})"
            print(
                f"{log.cycle:6d} | {log.pitch:6.1f} | {log.df:9.1f} | {log.dr:9.1f} | "
                f"{log.dl:8.1f} | {log.mode.value:8s} | {str(log.vibration):<24} | {cycle_str}"
            )

        # Summary check for specific scenario assertions
        if "Cycle Limit Auto-Mute" in name:
            initial_vib = logs[0].vibration.both
            final_vib = logs[-1].vibration.active
            assert initial_vib, "Front obstacle did not trigger vibration initially"
            assert not final_vib, "Cycle limit did not auto-mute stationary vibration"
            print("  >> [PASSED] Verified stationary vibration auto-mute after cycle limit.")
        elif "Raised Step / Curb" in name:
            assert logs[0].vibration.right and not logs[0].vibration.left
            print("  >> [PASSED] Verified down-mode raised step curb detection (Right Motor ON).")
        elif "Hole / Drop-off" in name:
            assert logs[0].vibration.left and not logs[0].vibration.right
            print("  >> [PASSED] Verified down-mode drop-off / hole detection (Left Motor ON).")
        elif "Clear Path" in name:
            assert not logs[0].vibration.active
            print("  >> [PASSED] Verified clear path silence (no false vibration).")
        elif "Sleep Mode" in name:
            assert logs[0].mode.value == "SLEEP" and not logs[0].vibration.active
            print("  >> [PASSED] Verified sensor sleep mode on 0cm distance.")

    print("\n" + "=" * 72)
    print("All functional verification scenarios completed successfully!")
    print("=" * 72)
    return 0


def interactive_mode() -> int:
    """Run an interactive prompt for manual testing."""
    print_banner()
    print("Interactive Mode. Enter sensor values or press Ctrl+C to exit.\n")

    controller = MotionGlassesController()
    print("Calibrating ground surface level (10 samples of 120cm)...")
    controller.set_level([120.0] * 10)
    print(f"Ground level calibrated to: {controller.level:.1f} cm\n")

    while True:
        try:
            line = input("Enter [df dr dl pitch] (e.g. 50 150 150 45): ").strip()
            if not line:
                continue
            parts = [float(x) for x in line.split()]
            if len(parts) == 3:
                df, dr, dl = parts
                pitch = 45.0
            elif len(parts) >= 4:
                df, dr, dl, pitch = parts[:4]
            else:
                print("Please enter at least 3 distance values (df dr dl).")
                continue

            mode, vib = controller.step(SensorReadings(df=df, dr=dr, dl=dl, pitch=pitch))
            cycle_info = (
                f"F={controller.cycle_f} R={controller.cycle_r} L={controller.cycle_l}"
            )
            print(
                f"Mode: {mode.value} | Pitch: {pitch:.1f} deg | "
                f"Vibration: {vib} | Cycles: {cycle_info}\n"
            )
        except (ValueError, IndexError):
            print("Invalid input format. Provide numeric values.")
        except (KeyboardInterrupt, EOFError):
            print("\nExiting interactive simulation.")
            break

    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Motion Glasses firmware logic simulator")
    parser.add_argument(
        "--interactive", "-i", action="store_true", help="Launch interactive test prompt"
    )
    args = parser.parse_args(argv)

    if args.interactive:
        return interactive_mode()
    return run_all_scenarios()


if __name__ == "__main__":
    sys.exit(main())
