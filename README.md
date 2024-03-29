<h3 align="center">cpubgline</h3>

<div align="center">
  Status: 🟢  Active - License: GPL v3
</div>

---

<p align="center">cpubgline shows the load of the CPU cores as Unicode^ bar graph in a single line
    <br/>
</p>


This tool prints a bar graph of the usage of each virtual CPU in a single line using Unicode block characters.

This line may look like `▁▂▅▁▆█  ▂▃ ▁` on a hexacore CPU with hyper threading.


## Installation

You should check the `install.sh` script before executing.
The default installation path is _/usr/local/bin_

```bash
# Download
git clone https://github.com/rstemmer/cpubgline.git
cd cpubgline

# Build
./build.sh

# Install
sudo ./install.sh
```

## Usage

The first time you call _cpubgline_ it initializes itself.
Then, each time you call it, it displays the usage of each CPU core during the last and the current call.

### Examples

```bash
watch -n 1 cpubgline
```

See the `tmux.conf` file for an example use-case. There the CPU usage gets printed into the status line.


## How it works

The tool first loads the previous CPU statistics, it got from the last run, stored in a temporary file.
The file is stored at */tmp/cpubglinehist_$PARENTPID*.
So when using _cpubgline_ in your command prompt or a status line,
it has a unique history and does not mix up the histories with other instances of the shell or window manager.

After reading the old statistics, it reads the current from _/proc/stats_.
The difference is used to calculate the usage:

```c
for(int i=0; i<numofcores; i++)
{
    double active, inactive;
    active   = currstats[i].active   - prevstats[i].active  ;
    inactive = currstats[i].inactive - prevstats[i].inactive;
    usage[i] = active / (active + inactive);
}
```
The active value is the sum of the counter _user_, _nice_, _sys_, _irq_, _softirq_ and _steal_.
The inactive value is the sum of _idle_ and _iowait_.
**Hint:** To get the _load_ instead of the _usage_, move the summand `iowait` to the `cpustat->acitve` sum.

Then the current statistics gets written into the history file.

Finally the bar graph gets generated by using the 8 block character of the Unicode and the space character for each virtual CPU.


