/*
 * cpubgline prints a one line unicode bargraph displaying the load of each CPU core
 * Copyright (C) 2017  Ralf Stemmer <ralf.stemmer@gmx.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <errno.h>

#define VERSION "1.1.0"

// Constant values
#ifndef STATPATH
#define STATPATH "/proc/stat"
#endif
#ifndef TMPPATH
#define TMPPATH "/tmp"
#endif

// ERROR-Codes
#define RETVAL_OK                     0  // 0 if no error occured
#define RETVAL_INVALIDARGUMENTS      -1  // invalid arguments
#define RETVAL_CANNOTOPEN            -2  // cannot open stats-file
#define RETVAL_UNEXPECTEDCONTENT     -3  // unexpected file content
#define RETVAL_UNEXPECTEDCPUCOUNT    -4  // number of CPUs does not match with the expected one

// Structs
typedef struct
{
    unsigned long long user;
    unsigned long long nice;
    unsigned long long sys;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;

    unsigned long long active;
    unsigned long long inactive;
} CPUSTAT;



int GetPreviousStats(const char *path, CPUSTAT *cpustats, unsigned int numofcores);
int SaveCurrentState(char *path, CPUSTAT *cpustats, unsigned int numofcores);
int GetCPUStats(CPUSTAT *cpustats, unsigned int numofcores);
void CalculateUsage(double *usage, CPUSTAT *prevstats, CPUSTAT *currstats, unsigned int numofcores);
void ShowUsage(double *usage, unsigned int numofcores);
void* SafeMAlloc(size_t size);



int main(int argc, char *argv[])
{
    int retval;

    // Collect some basic information
    unsigned int numofcores;
    pid_t parentpid;
    numofcores = get_nprocs_conf();
    parentpid  = getppid();

    // Generate tmp-file name
    char prevstatsfile[128];
    retval = snprintf(prevstatsfile, 128, TMPPATH "/cpubglinehist_%d", parentpid);
    if(retval == 0 || retval >= 128)
    {
        fprintf(stderr, "\e[1;31mGenerating tmp-file name failed!\e[0m\n");
        return EXIT_FAILURE;
    }

    // Read previous statistics if available
    CPUSTAT *prevstats;
    prevstats = SafeMAlloc(sizeof(CPUSTAT) * numofcores);
    retval    = GetPreviousStats(prevstatsfile, prevstats, numofcores);
    if(retval != RETVAL_OK)
    {
        free(prevstats);
        prevstats = NULL;
    }

    // Read current statistics
    CPUSTAT *currstats;
    currstats = SafeMAlloc(sizeof(CPUSTAT) * numofcores);
    retval    = GetCPUStats(currstats, numofcores);
    if(retval != RETVAL_OK)
        return EXIT_FAILURE;

    // Save current statsitics
    retval = SaveCurrentState(prevstatsfile, currstats, numofcores);
    if(retval != RETVAL_OK)
        return EXIT_FAILURE;

    // If there were previous statistics the CPU usage can be calculated and printed
    // Otherwise this time, there will be just an empty output

    // Calculate CPU usage
    double *coreusage;
    coreusage = SafeMAlloc(sizeof(double) * numofcores);
    CalculateUsage(coreusage, prevstats, currstats, numofcores);

    // Print core usage
    ShowUsage(coreusage, numofcores);

    return EXIT_SUCCESS;
}



int GetPreviousStats(const char *path, CPUSTAT *cpustats, unsigned int numofcores)
{
    if(path == NULL || cpustats == NULL)
        return RETVAL_INVALIDARGUMENTS;

    // Open file
    FILE *fp;
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "\e[1;31mfopen(%s, \"r\") failed with error: ", path);
        fprintf(stderr, "%s\e[0m\n", strerror(errno));
        return RETVAL_CANNOTOPEN;
    }

    // Read activity state from previous run
    size_t  n    = 0;
    char   *line = NULL;
    for(int i=0; i<numofcores; i++)
    {
        // read line from proc-file
        ssize_t readbytes;
        readbytes = getline(&line, &n, fp);
        if(readbytes < 0)
            break;                          // break loop if no more lines available

        // parse line
        int numitems;
        CPUSTAT *cpustat;
        cpustat = &cpustats[i];
        numitems = sscanf(line, "%llu %llu",
                &cpustat->active,
                &cpustat->inactive);
        if(numitems != 2)
        {
            free(line);
            fclose(fp);
            return RETVAL_UNEXPECTEDCONTENT;
        }
    }

    free(line);
    fclose(fp);
    return RETVAL_OK;
}



int SaveCurrentState(char *path, CPUSTAT *cpustats, unsigned int numofcores)
{
    if(path == NULL || cpustats == NULL)
        return RETVAL_INVALIDARGUMENTS;

    // Open File
    FILE *fp;
    fp = fopen(path, "w");
    if(fp == NULL)
    {
        fprintf(stderr, "\e[1;31mfopen(%s, \"w\") failed with error: ", path);
        fprintf(stderr, "%s\e[0m\n", strerror(errno));
        return RETVAL_CANNOTOPEN;
    }

    // Write (in)activity state for each CPU core
    for(int i=0; i<numofcores; i++)
    {
        CPUSTAT *cpustat;
        cpustat = &cpustats[i];
        fprintf(fp, "%llu %llu\n", cpustat->active, cpustat->inactive);
    }

    fclose(fp);
    return RETVAL_OK;
}



int GetCPUStats(CPUSTAT *cpustats, unsigned int numofcores)
{
    if(cpustats == NULL)
        return RETVAL_INVALIDARGUMENTS;

    // Open /proc/stats
    FILE *fp;
    fp = fopen(STATPATH, "rb");
    if(fp == NULL)
    {
        fprintf(stderr, "\e[1;31mfopen(%s, \"rb\") failed with error: ", STATPATH);
        fprintf(stderr, "%s\e[0m\n", strerror(errno));
        return RETVAL_CANNOTOPEN;
    }

    // Read CPU statistics
    int     cpunr = 0;
    size_t  n     = 0;
    char   *line  = NULL;
    while(1)
    {
        // Read line from proc-file
        ssize_t readbytes;
        readbytes = getline(&line, &n, fp); // allocate memory and read line from file
        if(readbytes < 0)
            break;                          // break loop if no more lines available
        
        // parse line
        int numitems;
        unsigned int cpuid;
        CPUSTAT *cpustat;
        cpustat  = &cpustats[cpunr];
        numitems = sscanf(line, "cpu%u %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &cpuid,
                &cpustat->user,
                &cpustat->nice,
                &cpustat->sys,
                &cpustat->idle,
                &cpustat->iowait,
                &cpustat->irq,
                &cpustat->softirq,
                &cpustat->steal,
                &cpustat->guest,
                &cpustat->guest_nice);

        if(numitems != 11)
            continue;                       // ignore non-parsable lines

        if(cpunr != cpuid)
        {
            fprintf(stderr, "unexpected CPU-Order in proc/stat!\n");
            free(line);
            return RETVAL_UNEXPECTEDCONTENT;
        }

        // do some calculation
        // guest and guest_nice are already included in user and nice
        //  see http://unix.stackexchange.com/q/178045/20626
        cpustat->active = 
            cpustat->user + 
            cpustat->nice + 
            cpustat->sys + 
            cpustat->irq + 
            cpustat->softirq + 
            cpustat->steal;
        cpustat->inactive = 
            cpustat->idle + 
            cpustat->iowait;

        // next cpu…
        cpunr++;
        if(cpunr >= numofcores)
            break;
    }

    free(line);
    fclose(fp);
    if(cpunr != numofcores)
    {
        fprintf(stderr, "%i CPUs expected. Only information about %i CPUs found!\n", numofcores, cpunr);
        return RETVAL_UNEXPECTEDCPUCOUNT;
    }
    return RETVAL_OK;
}



void CalculateUsage(double *usage, CPUSTAT *prevstats, CPUSTAT *currstats, unsigned int numofcores)
{
    if(usage == NULL)
        return;

    if(prevstats == NULL || currstats == NULL)
    {
        for(int i=0; i<numofcores; i++)
            usage[i] = -1;

        return;
    }

    for(int i=0; i<numofcores; i++)
    {
        double active, inactive;
        active   = currstats[i].active   - prevstats[i].active  ;
        inactive = currstats[i].inactive - prevstats[i].inactive;

        if((active + inactive) == 0)
            usage[i] = 0.0;
        else
            usage[i] = active / (active + inactive);
    }
}



void ShowUsage(double *usage, unsigned int numofcores)
{
    if(usage == NULL)
        return;

    const char *bars[9] = {" ","▁","▂","▃","▄","▅","▆","▇","█"};

    for(int i=0; i<numofcores; i++)
    {
        if(usage[i] < 0 || usage[i] > 1.0)
        {
            printf("E");    // Something went wrong
            continue;
        }

        int tilenr;
        tilenr = (int)(8 * usage[i] + 0.5);
        printf("%s", bars[tilenr]);
    }
}



void* SafeMAlloc(size_t size)
{
    void *memory = malloc(size);
    if(memory == NULL)
    {
        fprintf(stderr, "\e[1;31mmalloc(%lu); failed with error: ", size);
        fprintf(stderr, "\e[1;31m%s\e[0m\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    memset(memory, 0, size);
    return memory;
}



// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

