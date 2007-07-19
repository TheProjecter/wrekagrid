/**
* Copyright 
* Jens GUSTEDT (INRIA) jens.gustedt@loria.fr
* Emmanuel JEANNOT (INRIA) emmanuel.jeannot@loria.fr
* Marc THIERRY (INRIA, 2004)
* Louis-Claude CANON (INRIA, 2005)
* Olivier DUBUISSON (INRIA 2007) olivier.dubuisson@loria.fr
*
* This software is a computer program whose purpose is to define and 
* control the heterogeneity of a given platform by degrading CPU, network 
* or memory capabilities of each node composing this platform.
* The degradation is done remotely, without restarting the hardware. 
* The control is fine, reproducible and independent 
* (one may degrades CPU without modifying the network bandwidth).
* 
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security. 
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/

#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
  unsigned long mask;
  unsigned int len = sizeof (mask);
  if (argv[1] != NULL) {
    mask = pow (2, atoi (argv[1]));
    if (sched_setaffinity (0, len, &mask) < 0) {
      perror ("sched_setaffinity");
    }
  }
  //daemon(0,0);
  while (1);
  return 0;
}
