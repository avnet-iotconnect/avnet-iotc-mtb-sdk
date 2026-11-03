# /IOTCONNECT PSOC&trade; Example Projects

This document is the single index of all Avnet /IOTCONNECT example projects for Infineon PSOC&trade; devices
built on the [/IOTCONNECT ModusToolbox&trade; SDK](https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk) (this repository).

Every example lives in its own GitHub repository under the
[avnet-iotconnect](https://github.com/avnet-iotconnect) organization. The README of each example links back to this page,
so that this page can be kept as the one place to discover which examples exist and which kits they support.

Permanent link to this document:
`https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk/blob/main/PSOC_EXAMPLES.md`

**Contents**

- [PSOC&trade; Edge E84 Examples](#psoc-edge-e84-examples)
- [PSOC&trade; 6 Examples](#psoc-6-examples)
- [Supported Kits Cross-Reference](#supported-kits-cross-reference)
- [Related Repositories](#related-repositories)
- [Referencing This Document From an Example README](#referencing-this-document-from-an-example-readme)
- [Maintaining This Document](#maintaining-this-document)

---

## PSOC&trade; Edge E84 Examples

All PSOC&trade; Edge examples target the PSOC&trade; Edge E84 MCU and use the three-project structure
(CM33 secure, CM33 non-secure, CM55) where the CM55 core runs the DEEPCRAFT&trade; model and the
CM33 non-secure core runs the /IOTCONNECT application. Unless noted otherwise they build with
`GCC_ARM` and require ModusToolbox&trade; Tools 3.6 or later.

Supported kits:

- `KIT_PSE84_AI` - [PSOC&trade; Edge E84 AI Kit](https://www.infineon.com/KIT_PSE84_AI)
- `KIT_PSE84_EVAL_EPC2` - [PSOC&trade; Edge E84 Evaluation Kit](https://www.infineon.com/KIT_PSE84_EVAL)

| Example | Repository | Kits | Latest Release | Summary |
|:--------|:-----------|:-----|:---------------|:--------|
| PSOC&trade; Edge DEEPCRAFT&trade; Ready Models | [avnet-iotc-mtb-psoc-edge-rm](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-rm) | `KIT_PSE84_AI`, `KIT_PSE84_EVAL_EPC2` | 2.1.0 | Compile-time selectable DEEPCRAFT&trade; Ready Models: baby cry, cough, factory alarm and siren audio detection, radar gesture classification (AI Kit only) and BMI270 fall detection. Pre-built hex available for the AI Kit. |
| PSOC&trade; Edge DEEPCRAFT&trade; Baby Monitor | [avnet-iotc-mtb-psoc-edge-baby-monitor](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-baby-monitor) | `KIT_PSE84_AI`, `KIT_PSE84_EVAL_EPC2` | 1.1.1 | DEEPCRAFT&trade; Studio baby cry detection starter model using the PDM microphone. Reference PSOC&trade; Edge implementation for this SDK. |
| PSOC&trade; Edge DEEPCRAFT&trade; Motion | [avnet-iotc-mtb-psoc-edge-motion](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-motion) | `KIT_PSE84_AI`, `KIT_PSE84_EVAL_EPC2` | 1.1.1 | DEEPCRAFT&trade; Studio human activity detection starter model (sitting, standing, walking, running, jumping) using the BMI270 IMU. Requires Tools 3.7 or later. |
| PSOC&trade; Edge DEEPCRAFT&trade; Voice Assistant | [avnet-iotc-mtb-psoc-edge-voice-assistant](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-voice-assistant) | `KIT_PSE84_AI`, `KIT_PSE84_EVAL_EPC2` | 3.0.1 | Wake word ("OK Infineon") and spoken command recognition driving a Smart Lights demo with room light levels reported to /IOTCONNECT. Builds with the `LLVM_ARM` toolchain only. The bundled audio middleware has a time-limited evaluation license. |
| PSOC&trade; Edge Secure IoT with PSA and TF-M | [avnet-iotc-mtb-psoc-edge-psa-tfm](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-psa-tfm) | `KIT_PSE84_EVAL_EPC2` | 1.4.0 | Four-project structure with bootloader, signed images and OTA. Derives the device private key from the Hardware Unique Key and stores the certificate in PSA Internal Trusted Storage. Supports the fall detection Ready Model and a non-AI board orientation mode. Pre-built firmware available via its QUICKSTART. |
| PSOC&trade; Edge AI Vision WebRTC | [avnet-iotc-mtb-psoc-edge-aivision-webrtc](https://github.com/avnet-iotconnect/avnet-iotc-mtb-psoc-edge-aivision-webrtc) | `KIT_PSE84_AI` | 1.1.1 | DEEPCRAFT&trade; vision model (rock, paper, scissors gestures) with H.264 encoding and live video streaming to /IOTCONNECT over Amazon Kinesis WebRTC. Uses the OV7675 DVP camera included with the AI Kit and an optional DSI display. Pre-built hex available. |

## PSOC&trade; 6 Examples

The PSOC&trade; 6 examples build with `GCC_ARM` and use the single-project ModusToolbox&trade; application structure.
Over-the-air updates are supported by the Basic Example only.

| Example | Repository | Kits | Latest Release | Summary |
|:--------|:-----------|:-----|:---------------|:--------|
| Basic Example | [avnet-iotc-mtb-basic-example](https://github.com/avnet-iotconnect/avnet-iotc-mtb-basic-example) | `CY8CKIT-062-WIFI-BT`, `CY8CPROTO-062-4343W`, `CY8CKIT-062S2-43012`, `CY8CKIT-062S2-AI` (tested). `CYW9P62S1-43438EVB-01`, `CYW9P62S1-43012EVB-01`, `CY8CEVAL-062S2-LAI-4373M2`, `CY8CEVAL-062S2-MUR-43439M2`, `CY8CPROTO-062S3-4343W` (untested) | 7.1.0 | Reference PSOC&trade; 6 implementation for this SDK: Wi-Fi, MQTT telemetry, cloud-to-device commands and OTA. Host of the shared DEVELOPER_GUIDE used by the other PSOC&trade; 6 examples. Use v7.1.0 or later. |
| XENSIV&trade; / OPTIGA&trade; Example | [avnet-iotc-mtb-xensiv-example](https://github.com/avnet-iotconnect/avnet-iotc-mtb-xensiv-example) | `KIT_CSK_PASCO2` (XENSIV&trade; Connected Sensor Kit with PAS CO2 sensor) | 5.0.1 | PSOC&trade; 62 based sensor kit using the OPTIGA&trade; Trust M secure element for device authentication. Runs with or without the PAS CO2 sensor board attached. Published in the ModusToolbox&trade; manifest as "Avnet IoTConnect Optiga Example". Pre-built binary via QUICKSTART. |
| AI IMU Example (Human Activity Detection) | [avnet-iotc-mtb-ai-imu-example](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ai-imu-example) | `CY8CKIT-062S2-AI` | 3.0.2 | Imagimob human activity detection model (sitting, standing, walking, running, jumping) with raw accelerometer data reported to /IOTCONNECT. Requires the ModusToolbox&trade; Machine Learning pack. |
| AI Baby Monitor | [avnet-iotc-mtb-ai-baby-monitor](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ai-baby-monitor) | `CY8CKIT-062S2-AI` | 3.3.0 | ModusToolbox&trade; Machine Learning (MTBML) baby cry detection from the PDM/PCM microphone, with detection threshold and reporting interval commands. Use v3.3.0 or later. |
| AI Imagimob Ready Models | [avnet-iotc-mtb-ai-imagimob-rm](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ai-imagimob-rm) | `CY8CKIT-062S2-AI`, `CY8CKIT-062S2-43012` (audio models only) | 1.1.0 | Compile-time selectable Imagimob Ready Models: siren, baby cry, alarm, cough and snore audio detection plus radar gesture detection (push, swipe up/down/left/right) on the AI Kit. Use v1.1.0 or later. |
| AI Fall Detection | [avnet-iotc-mtb-ai-fall-detection](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ai-fall-detection) | `CY8CKIT-062S2-AI` | No tagged release (use `main`) | MTBML fall detection Ready Model using the on-board IMU, intended for wrist-mounted use with /IOTCONNECT rules for notifications. Not listed in the ModusToolbox&trade; Project Creator; clone and use "Browse for Application". |

## Supported Kits Cross-Reference

| Kit (make variable `TARGET`) | Device family | Examples |
|:-----------------------------|:--------------|:---------|
| `KIT_PSE84_AI` | PSOC&trade; Edge E84 | Ready Models, Baby Monitor, Motion, Voice Assistant, AI Vision WebRTC |
| `KIT_PSE84_EVAL_EPC2` | PSOC&trade; Edge E84 | Ready Models, Baby Monitor, Motion, Voice Assistant, Secure IoT with PSA and TF-M |
| `CY8CKIT-062S2-AI` | PSOC&trade; 62S2 | Basic Example, AI IMU Example, AI Baby Monitor, AI Imagimob Ready Models, AI Fall Detection |
| `CY8CKIT-062S2-43012` | PSOC&trade; 62S2 | Basic Example, AI Imagimob Ready Models (audio models only) |
| `CY8CKIT-062-WIFI-BT` | PSOC&trade; 62 | Basic Example |
| `CY8CPROTO-062-4343W` | PSOC&trade; 62 | Basic Example |
| `CY8CPROTO-062S3-4343W` | PSOC&trade; 62S3 | Basic Example (untested) |
| `CY8CEVAL-062S2-LAI-4373M2`, `CY8CEVAL-062S2-MUR-43439M2` | PSOC&trade; 62S2 | Basic Example (untested) |
| `CYW9P62S1-43438EVB-01`, `CYW9P62S1-43012EVB-01` | PSOC&trade; 62S1 | Basic Example (untested, 43438 variant is EOL) |
| `KIT_CSK_PASCO2` | PSOC&trade; 62 with OPTIGA&trade; Trust M | XENSIV&trade; / OPTIGA&trade; Example |

## Related Repositories

These repositories are not examples themselves but are part of the PSOC&trade; /IOTCONNECT ecosystem:

| Repository | Purpose |
|:-----------|:--------|
| [avnet-iotc-mtb-sdk](https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk) | This SDK. Shared /IOTCONNECT client used by every example above (latest release 8.2.1). |
| [iotc-c-lib](https://github.com/avnet-iotconnect/iotc-c-lib) | Platform-independent /IOTCONNECT C library pulled in as a submodule of the SDK. |
| [avnet-iotc-mtb-ce-manifest](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ce-manifest) | ModusToolbox&trade; code example manifest. Lists the examples that appear in the Project Creator, along with their released versions and kit requirements. |
| [avnet-iotc-mtb-mw-manifest](https://github.com/avnet-iotconnect/avnet-iotc-mtb-mw-manifest) | ModusToolbox&trade; middleware manifest for the SDK. |
| [avnet-iotc-mtb-super-manifest](https://github.com/avnet-iotconnect/avnet-iotc-mtb-super-manifest) | ModusToolbox&trade; super manifest that ties the code example and middleware manifests together. |
| [iotc-mtb-build-docker-image](https://github.com/avnet-iotconnect/iotc-mtb-build-docker-image) | Docker image used to build the ModusToolbox&trade; examples in CI. |

For the full list of /IOTCONNECT-enabled Infineon kits beyond ModusToolbox&trade;, see
[avnet-iotconnect.github.io/partners/infineon](https://avnet-iotconnect.github.io/partners/infineon/).

## Referencing This Document From an Example README

Every PSOC&trade; example README should link to this page so readers can find the other examples.
Add the following section near the end of the example's `README.md`
(for example, replacing or alongside the existing "Other /IOTCONNECT-enabled Infineon Kits" section):

```markdown
## Other /IOTCONNECT PSOC&trade; Examples

See the full list of /IOTCONNECT PSOC&trade; Edge E84 and PSOC&trade; 6 examples in the
[/IOTCONNECT PSOC&trade; Example Projects](https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk/blob/main/PSOC_EXAMPLES.md) index.
```

Status of the back-references (update this table as the example READMEs are changed):

| Repository | README links here |
|:-----------|:------------------|
| avnet-iotc-mtb-sdk | Yes |
| avnet-iotc-mtb-psoc-edge-rm | Pending |
| avnet-iotc-mtb-psoc-edge-baby-monitor | Pending |
| avnet-iotc-mtb-psoc-edge-motion | Pending |
| avnet-iotc-mtb-psoc-edge-voice-assistant | Pending |
| avnet-iotc-mtb-psoc-edge-psa-tfm | Pending |
| avnet-iotc-mtb-psoc-edge-aivision-webrtc | Pending |
| avnet-iotc-mtb-basic-example | Pending |
| avnet-iotc-mtb-xensiv-example | Pending |
| avnet-iotc-mtb-ai-imu-example | Pending |
| avnet-iotc-mtb-ai-baby-monitor | Pending |
| avnet-iotc-mtb-ai-imagimob-rm | Pending |
| avnet-iotc-mtb-ai-fall-detection | Pending |

## Maintaining This Document

- When a new PSOC&trade; example repository is created, add it to the matching table above, add its kits to the
  cross-reference table, add the back-reference snippet to its README and add a row to the status table.
- When an example is released, update its "Latest Release" column. The released version should match the
  version published in
  [avnet-iotc-mtb-ce-manifest](https://github.com/avnet-iotconnect/avnet-iotc-mtb-ce-manifest/blob/main/avnet-iotc-mtb-ce-manifest-fv2.xml).
- When an example adds or drops kit support, update both the example row and the cross-reference table.
- Kit names use the ModusToolbox&trade; `TARGET` spelling so they can be searched for directly in the manifest and Makefiles.
