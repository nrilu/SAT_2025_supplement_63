
# Supplement for SAT'25 Submission "Streamlining Distributed SAT Solver Design"

## Software

* Mallob(Sat): https://github.com/domschrei/mallob/tree/152e7c4c07fbdafccb66f05acd289f7fc3287336
* PL-PRS-BVA-KISSAT (a.k.a. `painless-2`): fetched from SAT Competition website: https://satcompetition.github.io/2024/downloads/solvers/parallel.tar.xz

## Benchmarks

The download URLs of the used benchmarks are provided in `data/benchmarks.uri`.

Meta data of the instances can be found in `data/instance-to-*.txt`. The division into testing and scaling benchmarks is given via `data/benchmarks-{testing,scaling}.txt`.

## Results

Central experimental data can be found at `data`.

* `qtimes*.txt`: Qualified running times of a run, featuring the instance hash + name, result, and running time at each line.
* `rtprofile*.txt`: Gathered profiling data of a run, where each line begins with the measured procedure (e.g., vivification) and then features two arguments for each instance: The running time, and the share reported by Kissat's profiling.

## Plotting 

A simple python program is provided to quickly plot and compare different `qtimes*.txt` files.
* `./plot_cumul_qtimes.py qtimes1.txt qtimes2.txt qtimes3.txt ...` 

## Re-running Experiments

To run experiments, we used the scripting available at `scripts/slurm/` in the above Mallob repository. We refer to the README in that directory for further instructions.

In our experiments, we deviate from the default Mallob configuration given in `scripts/slurm/sbatch.sh` as follows:

* Default run with full ("naïve") pre--/inprocessing: `-satsolver=k -mono-app=SAT`
* Search-only run: `-satsolver=k_ -mono-app=SAT`
* LBD settings: `-div-phases=0 -div-seeds=0`
    * Shuffling: `-scramble-lbds=1`
    * Original: (default)
    * Deactivated: `-ilbd=0 -rlbd=3`
    * Triangle: `-randlbd=2 -ilbd=0 -rlbd=3`
    * Uniform: `-randlbd=1 -ilbd=0 -rlbd=3`
* Diversification: `-rlbd=3 -ilbd=0`
    * none: `-div-phases=0 -div-seeds=0`
    * seeds: `-div-phases=0`
    * seeds + phases: (default)
* New preprocessing enabled: `-satsolver=k_ -mono-app=SATWITHPRE -rlbd=3 -ilbd=0`
    * j0 + jp: `-pb=2 -pjp=1 -pef=1`
    * shift j0 -> jp: `-pb=1 -pjp=999999 -pef=2`
    * evict j0: `-pb=0 -pjp=999999 -pef=1`
    * jp only: `-pb=-1 -pjp=999999 -pef=2`
* Scaling:
    * K: `-satsolver=k -mono-app=SAT`
    * KCL: `-satsolver=kcl -mono-app=SAT`
    * Ours: `-satsolver=k_ -mono-app=SATWITHPRE -pb=1 -pjp=999999 -pef=2 -rlbd=3 -ilbd=0`
* Reduce study (appendix): `-satsolver=k_ -mono-app=SATWITHPRE -pb=1 -pjp=999999 -pef=2`
    * Default: (default)
    * Point: `-div-reduce=1 -reduce-min=0 -reduce-max=1000 -reduce-delta=0`
    * Range: `-div-reduce=1 -reduce-min=0 -reduce-max=1000 -reduce-delta=200`
    * Gaussian: `-div-reduce=3 -reduce-min=300 -reduce-max=980 -reduce-mean=700 -reduce-stddev=150`
* Decay study (appendix): `-satsolver=k_ -mono-app=SATWITHPRE -pb=1 -pjp=999999 -pef=2`
    * Default: (default)
    * Uniform [1,50]: `-div-noise=1 -decay-distr=2 -decay-min=1 -decay-max=50`
    * Uniform [1,200]: `-div-noise=1 -decay-distr=2 -decay-min=1 -decay-max=200`
    * Uniform [50,200]: `-div-noise=1 -decay-distr=2 -decay-min=50 -decay-max=200`   

We ran Painless as in the SAT Competition 2024 Parallel track Docker setup but with `-t=48`. (We tested that performance is in fact better than with `-t=32` on the used hardware.)


