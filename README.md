# MeaterESPhome

Hello — Development workspace for an ESPHome Meater component

What’s included
- `custom_components/meater/` — C++ skeleton for the ESPHome component
- `example/config.yaml` — example ESPHome configuration for quick tests
- `requirements.txt` — recommended Python packages

Quick setup (Windows PowerShell)
```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
# validate the configuration
esphome config example/config.yaml
# build/upload to device (or just compile)
esphome run example/config.yaml
```

Notes for component development
- The component source is under `custom_components/meater/`.
- The `example/config.yaml` shows how to include the component header and register the component.
- After editing the C++ sources, run `esphome run example/config.yaml` — ESPHome will rebuild the firmware.

Optional Meater connection behavior
- The component supports optional `mac` or `id` configuration and a `connect_on_demand` mode.
- The `example/config.yaml` includes a `Meater: Connect Once` action which calls `meater_instance->request_connect()`.
- A global pointer `meater_instance` is set automatically when the component instance is created in the lambda (for the example usage).

Battery note
- Recommended behavior: keep the Meater disconnected and connect only on demand to preserve battery.
- The BLE connect/disconnect logic in `meater.cpp` is implemented to support this workflow and parse Meater characteristics.

Local test environment (Docker / Windows / WSL2)
- On Windows it is recommended to use WSL2 (Ubuntu) with Docker Desktop so the container can access USB/serial devices.
- The repository includes `docker-compose.yml` and a helper PowerShell script `run_esphome.ps1`.

Quick start (WSL2 / Linux):
```bash
# from the project root
docker-compose run --rm esphome esphome config example/config.yaml
docker-compose run --rm esphome esphome run example/config.yaml
```

Quick start (Windows PowerShell):
```powershell
# Validate config
.\run_esphome.ps1 -Cmd config -File example/config.yaml
# Build and upload (if a device is attached and Docker has serial access)
.\run_esphome.ps1 -Cmd run -File example/config.yaml
```

Notes about serial ports on Windows
- Docker for Windows does not usually map `COMx` ports directly into containers. Use WSL2 and map `/dev/ttyUSB*`, or use a host-side flasher tool for writing firmware.
- If running on Linux/WSL and you want to expose a serial device into the container, add a `devices` entry to `docker-compose.yml` (e.g. `/dev/ttyUSB0:/dev/ttyUSB0`).

Install as an external component

If you prefer to use this component directly from GitHub in your production ESPHome environment, add the `external_components` section to your device YAML and point it at this repository. Example:

```yaml
external_components:
	- source: "github://pcespiva/MeaterESPhome@main"
		components: [ meater ]
```

You can pin to a tag or commit instead of `main`, for example `@v1.0.0` or `@<commit-sha>`.

After adding `external_components`, use the `meater:` block in your config as shown in `example/config.yaml`.

What to do next
- I can add a `Makefile` or `.ps1` shortcuts for common tasks (build/config/run) if you'd like.
- I can also run `esphome config` in Docker from this machine if you allow me to execute Docker here, otherwise I will continue with file edits only.

Further steps
- Implement Meater BLE communication and parsing in `meater.cpp` (already started).
- Add logging and unit tests if desired.
