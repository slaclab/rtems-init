# Supported Hardware

## Support Matrix

| Board Name | BSP      | Networking Stack | Notes |
|------------|----------|------------------|-------|
| mvme6100   | beatnik  | libbsd           | |
| mvme3100   | mvme3100 | libbsd           | `tsec` network driver requires libbsd patches |
| mvme5500   | beatnik  | libbsd           | `em` network driver currently does not work |
| uC5282     | uC5282   | legacy           | |
| pc686      | i386     | libbsd           | For testing in Qemu |
