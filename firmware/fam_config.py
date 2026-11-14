# TikoOS native profile: share the upstream service/UI stacks, no duplicated system apps.
MANIFEST_ROOTS = ["components", "applications"]
APP_SOURCE_OVERRIDES = {"desktop":"applications", "desktop_settings":"applications", "storage":"applications"}
APPS = ["input","notification","gui","dialogs","locale","cli","cli_vcp",
        "storage","storage_start","power","power_start","power_settings",
        "loader","loader_start","namechanger_srv","spoofing_settings",
        "notification_settings","interface_settings","desktop_settings","desktop",
        "archive","about","bt_settings","clock","wifi","tiko"]
EXTRA_EXT_APPS = []
TARGET_HW = 32
AUTORUN_APP = "TikoOS"
