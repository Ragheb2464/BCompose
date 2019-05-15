# How To Use
## Minimum Requirements
  - Linux
    - gcc version 8.2.0 or Apple LLVM version 10.0.1 (clang-1001.0.46.4)
    - IloCplex 12.8.0.0
    - Boost libraries
    - Python 2.6.6
## How It Works
  - BCompose is to solve pre-decomposed problems.
  - The user needs to export the master and subproblem(s) following the provided guidelines.
    - This gives the user a full flexibility in decomposing the problem and exploiting its special structures.
  - The exported models must be placed in 'models' directory.
  - After doing this, refer to 'How To Run' to optimizer the problem.
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
  - All the examples are created using C++.
## How To Run
  - After exporting the proper formulations for the master (MP) and subproblem (SP) into the "models/" directory, run following command:
    ```
    python run_me.py
    ```
  - or:
    ```
     ./BCompose  --model_dir=? --current_dir=?
    ```
## Tune
 - Some of the main features of BCompose can be tuned via the  global variables in the 'solver_setting.h' file.

#ToBeCompleted
