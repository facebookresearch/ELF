Python Implementation of Concurrent Game Environemnt in ELF
============================================================
Many games do not have C interface, which restricts the usage of ELF. Therefore we also provide a simple Python implementation to run concurrent game environments, using Python multiprocessing library. 

For sample code, please check `./ex_elfpy.py`. You can put any environment into it, e.g, OpenAI gym, that wraps one game instance with one Python interface.


