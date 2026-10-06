#!/usr/bin/env python3
"""Run smm with --json and reduce NDJSON to one validated result (stdlib only)."""

import argparse
import json
import subprocess
import sys
from typing import Any, Dict, Optional, Sequence


def parse_output(stdout: str) -> Dict[str, Any]:
    events = []
    result = None
    lines = [line for line in stdout.lstrip("\ufeff").splitlines() if line.strip()]
    if not lines:
        raise ValueError("SMM emitted no JSON output; inspect stderr and the exit code.")
    for number, line in enumerate(lines, 1):
        try:
            item = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError("Invalid JSON on output line %d: %s" % (number, error.msg)) from error
        if not isinstance(item, dict):
            raise ValueError("Output line %d is not a JSON object." % number)
        if result is not None:
            raise ValueError("Unexpected output after the final SMM result.")
        if item.get("event") == "progress":
            events.append(item)
        elif isinstance(item.get("ok"), bool):
            result = item
        else:
            raise ValueError("Output line %d is neither progress nor a final result." % number)
    if result is None:
        raise ValueError("SMM emitted progress without a final result.")
    if result["ok"] and "data" not in result:
        raise ValueError("Successful SMM result has no data field.")
    if not result["ok"] and not isinstance(result.get("error"), dict):
        raise ValueError("Failed SMM result has no error object.")
    return {"result": result, "progress_count": len(events),
            "last_progress": events[-1] if events else None}


def business_failure(result: Dict[str, Any]) -> Optional[str]:
    data = result.get("data")
    if not isinstance(data, dict):
        return None
    if result.get("command") == "deploy":
        deployment = data.get("result", {})
        if not isinstance(deployment, dict):
            return "Deployment result is not an object."
        if data.get("success") is False or deployment.get("success") is False or deployment.get("failed_files", 0):
            return "Deployment reports unsuccessful or failed files; inspect result.data.result."
    if result.get("command") == "restore":
        restored = data.get("result", {})
        if not isinstance(restored, dict):
            return "Restore result is not an object."
        if restored.get("success") is False:
            return "Restore reports failure; inspect result.data.result."
    if result.get("command") == "remove" and data.get("removed") is False:
        return "The requested mod was not removed."
    # A negative doctor health result is a completed diagnosis, not a protocol failure.
    return None


def invoke(executable: str, args: Sequence[str], timeout: float) -> Dict[str, Any]:
    arguments = list(args)
    if "--json" not in arguments:
        arguments.insert(0, "--json")
    command = [executable] + arguments
    try:
        completed = subprocess.run(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, timeout=timeout, shell=False)
    except FileNotFoundError as error:
        return {"ok": False, "exit_code": 127, "argv": command,
                "error": {"code": "executable_not_found", "message": str(error)}}
    except subprocess.TimeoutExpired as error:
        return {"ok": False, "exit_code": 124, "argv": command,
                "error": {"code": "timeout", "message": "SMM exceeded %.1f seconds. Inspect partial changes before retrying." % timeout},
                "stdout": (error.stdout or b"").decode("utf-8", errors="replace"),
                "stderr": (error.stderr or b"").decode("utf-8", errors="replace")}
    except OSError as error:
        return {"ok": False, "exit_code": 126, "argv": command,
                "error": {"code": "process_error", "message": str(error)}}

    stdout = completed.stdout.decode("utf-8", errors="replace")
    stderr = completed.stderr.decode("utf-8", errors="replace")
    summary = {"ok": False, "exit_code": completed.returncode, "argv": command, "stderr": stderr}
    try:
        parsed = parse_output(stdout)
    except ValueError as error:
        summary.update({"stdout": stdout, "error": {"code": "output_error", "message": str(error)}})
        return summary
    summary.update(parsed)
    failure = business_failure(parsed["result"])
    summary["ok"] = completed.returncode == 0 and parsed["result"]["ok"] and failure is None
    if failure:
        summary["error"] = {"code": "business_failure", "message": failure}
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", default="smm", help="SMM executable path or name on PATH")
    parser.add_argument("--timeout", type=float, default=120, help="Seconds before aborting (default: 120)")
    parser.add_argument("args", nargs=argparse.REMAINDER, help="SMM arguments after --")
    options = parser.parse_args()
    args = options.args[1:] if options.args[:1] == ["--"] else options.args
    if not args:
        parser.error("Provide an SMM command after --.")
    if options.timeout <= 0:
        parser.error("--timeout must be positive.")
    summary = invoke(options.exe, args, options.timeout)
    # Keep output valid on terminals whose legacy encoding cannot represent local names.
    print(json.dumps(summary, ensure_ascii=True, indent=2))
    if summary["ok"]:
        return 0
    return summary["exit_code"] if summary["exit_code"] in (1, 2, 124, 126, 127) else 1


if __name__ == "__main__":
    sys.exit(main())
