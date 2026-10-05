from __future__ import annotations

import argparse
import json
from dataclasses import asdict
from enum import Enum

from .manifest import ManifestError, load_manifest
from .models import VerificationStatus
from .pipeline import audit_manifest, generation_plan


def _json_default(value: object) -> object:
    if isinstance(value, Enum):
        return value.value
    raise TypeError(f"Object of type {type(value).__name__} is not JSON serializable")


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="legacybox")
    sub = parser.add_subparsers(dest="command", required=True)

    plan = sub.add_parser("plan", help="emit a deterministic generation plan")
    plan.add_argument("manifest")

    audit = sub.add_parser("audit", help="audit a living-dex manifest")
    audit.add_argument("manifest")
    audit.add_argument(
        "--require-legality",
        action="store_true",
        help="fail if any entry is not PASSED by the configured legality backend",
    )
    audit.add_argument("--json", action="store_true", dest="json_output")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)

    try:
        manifest = load_manifest(args.manifest)
    except ManifestError as exc:
        print(f"ERROR: {exc}")
        return 2

    if args.command == "plan":
        print(json.dumps(generation_plan(manifest), indent=2))
        return 0

    results = audit_manifest(manifest)
    if args.json_output:
        print(json.dumps([asdict(item) for item in results], indent=2, default=_json_default))
    else:
        for item in results:
            print(
                f"{item.entry_key}: structure={item.structural_status.value} "
                f"legality={item.legality.status.value} "
                f"backend={item.legality.backend}"
            )
            for message in item.structural_messages + item.legality.messages:
                print(f"  - {message}")

    structural_failure = any(
        item.structural_status is VerificationStatus.FAILED for item in results
    )
    legality_not_passed = any(
        item.legality.status is not VerificationStatus.PASSED for item in results
    )

    if structural_failure:
        return 2
    if args.require_legality and legality_not_passed:
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
