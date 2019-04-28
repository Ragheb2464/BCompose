# How To Use
## Minimum Requirements
  - Linux
    - gcc version 8.2.0
    - IloCplex 12.8.0.0
    - Boost libraries
    - Python 2.6.6
    - Linux OS (e.g., Oracle Linux Server 6.9)
  - Mac
    - g++ 8.2 or Apple LLVM version 10.0.1 (clang-1001.0.46.4)
    - IloCplex 12.8.0.0
    - Boost libraries
    - Python 2.6.6
    - Mac OS (e.g., Mojave)
## How It Works
  - BCompose is to solve pre-decomposed MILP problems. This means that the user needs to export the master and subproblem(s) according the provided guidelines.
    - This gives the user a full flexibility in decomposing the problem and exploiting its special structures.
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
    - python run_me.py
  - or:
    - ./BCompose  --model_dir=? --current_dir=?


#ToBeCompleted
