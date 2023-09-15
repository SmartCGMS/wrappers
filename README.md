# <img src="https://diabetes.zcu.cz/img/icon.png" width="24" height="24" /> SmartCGMS - wrappers
This repository contains wrappers for SmartCGMS modules, that wrap a specific subset of features for a single purpose, e.g., to offer a simple API for a complex tasks to a foreign language environment.

SmartCGMS software architecture and framework.
Project homepage: [diabetes.zcu.cz](https://diabetes.zcu.cz/smartcgms)

## Repository structure

- `game-wrapper` - wraps the basic logic, that is required by our gaming front-ends, such as Icarus has Diabetes
- `interop-inspector` - wraps the COM interface logic and other, non-standard data type handler functions into a set of C-based API calls

## License

The SmartCGMS software and its components are distributed under the Apache license, version 2. When publishing any derivative work or results obtained using this software, you agree to cite the following paper:

_Tomas Koutny and Martin Ubl_, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science, Volume 177, pp. 354-362, 2020

See attached LICENSE file for full licencing information.

|![University of West Bohemia](https://www.zcu.cz/en/assets/logo.svg)|![Department of Computer Science and Engineering](https://www.kiv.zcu.cz/site/documents/verejne/katedra/dokumenty/dcse-logo-barevne.png)|
|--|--|