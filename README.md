# How To Use
## Minimum Requirements
  - A complier compatible with c++17:
    - e.g., gcc version 8.2.0 or Apple LLVM version 10.0.1 (clang-1001.0.46.4)
  - IloCplex 12.8.0.0
  - Boost libraries
  - Python3 [inside the code, "python3" is used to run some py modules.]
    - import pickle
    - import sys
    - import pandas as pd
## How It Works
  - BCompose is to solve pre-decomposed problems.
  - The user needs to export the master and subproblem(s) following the provided guidelines.
    - This gives the user a full flexibility in decomposing the problem and exploiting its special structures.
  - The exported models must be placed in 'models' directory.
  - After doing this, refer to 'How To Run' to optimize the problem.
## Exporting the pre-decomposed models
  - BCompose generally works with the subproblem formulation of the generalized Benders decomposition.
    - More details can be found in our paper entitled "The Benders Dual Decomposition Method".
  - There are few constraint and variable naming conventions which the user needs to follow in order for BCompose to operate normally.
  - For now, please refer to the provided examples under the 'examples' folder for more details on such conventions. I will soon provide a complete guideline.
## Examples
  - Examples on how to export the pre-decomposed problems are available for the following problems:
    - Fixed-charge capacitated facility location problem:
      - stochastic/deterministic version;
      - complete/incomplete recourse version;
      - strong/weak formulation;
    - Stochastic fixed-charge multi-commodity capacitated network design problem;
      - strong/weak formulation;
    - Stochastic network interdiction problem;
    - Covering location problem;
  - All the examples are created in C++.
## How To Run
  - After exporting the proper formulations for the master (MP) and subproblem (SP) into the "models/" directory, run following command:
    ```
    python3 run_me.py
    ```
  - or:
    ```
     ./BCompose  --model_dir=? --current_dir=?
    ```
## Tune
 - Some of the main features of BCompose can be tuned via the  global variables in the 'solver_setting.h' file.

#ToBeCompleted
