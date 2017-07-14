ELF
==========

ELF is an end-to-end, extensive, lightweight and flexible platform for game research. 

Code Hierarchy
--------------
ELF is organized as follows.

.. image:: /imgs/hierarchy.png

* The folder ``elf`` contains the game-independent codebase to handle concurrent simulation.
* The folder ``atari`` contains the wrapper and model for Atari games. ALE is required (`ALE <https://github.com/mgbellemare/Arcade-Learning-Environment>`_).  
* The folder ``rts/engine`` contains the RTS engine. ``rts/game_MC``, ``rts/game_CF`` and ``rts/game_TD`` are the three games built on top of the engine.  

Components
----------
Python APIs:

* :doc:`rlpytorch`. The Reinforcement Learning backend built from PyTorch.
* :doc:`utils`. The utilities.
* :doc:`wrapper-python`. The Python interface of wrapper.

C++ APIs:

* :doc:`wrapper`. The wrapper toolbox that make a single-threaded game environment into a library that supports multithreading. 

RTS: 

* :doc:`RTS-engine`. The miniature RTS engine. 
