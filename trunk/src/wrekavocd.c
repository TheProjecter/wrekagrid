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

#include <dirent.h>
#include <fcntl.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "wrekavocd.h"
#include "utils.h"
#include "timings.h"

#include <sys/mman.h>   /* Memory locking functions */
#include <errno.h>
#include <linux/unistd.h>       /* les macros _syscallX */
#include <linux/kernel.h>

#define TCP_BUFFER_SIZE (32*1024)
#define TCP_TIMING_SIZE (256*1024)
//#define TCP_TIMING_SIZE (32*1024)
/*TCP_PIECE_SIZE must divide TCP_TIMING_SIZE*/
#define TCP_PIECE_SIZE (TCP_TIMING_SIZE/32)

int burn = 0;			//specify if we use burn or cpulim
char *dataMemoryLocked = NULL;
long lMemorySize = 0;

/**
* \brief function to lock memory
	*
	* \param addr Address of memory to lock
	* \param size Size of memory to lock
	* \return 0 if success
	* \see ---
	*
*/
	int lock_memory(char *addr,size_t size)
{
	unsigned long page_offset, page_size;

	page_size = sysconf(_SC_PAGE_SIZE);
	page_offset = (unsigned long) addr % page_size;

	addr -= page_offset;  /* Adjust addr to page boundary */
	size += page_offset;  /* Adjust size with page_offset */

	return ( mlock(addr, size) );  /* Lock the memory */
}

/**
* \brief function to unlock memory
	*
	* \param addr Address of memory to unlock
	* \param size Size of memory to unlock
	* \return 0 if success
	* \see ---
	*
*/
	int unlock_memory(char *addr,size_t  size)
{
	unsigned long  page_offset, page_size;

	page_size = sysconf(_SC_PAGE_SIZE);
	page_offset = (unsigned long) addr % page_size;
	addr -= page_offset;  /* Adjust addr to page boundary */
	size += page_offset;  /* Adjust size with page_offset */

	return ( munlock(addr, size) );  /* Unlock the memory */
}

/**
* \brief function to get MTU of interface: Is it really the MTU???
	*
	* \param none
	* \return MTU
	* \see ---
	*
*/
	long getMTU(void)
{
	FILE *pFile = NULL;
	char strBuff[255];
	char *pos, *pos1;
	/*pFile = fopen("/proc/sys/net/ipv4/tcp_wmem","r");
	fgets(strBuff,sizeof(strBuff),pFile);
	if (pFile != NULL)
		fclose(pFile);
	if (strlen(strBuff) > 0)
		//On prend la deuxième valeur
	{
		pos = strchr(strBuff,0x09);
		if (pos != NULL)
		{
			pos ++;
			pos1 = strchr(pos,0x09);
			if (pos1 != NULL)
				pos1[0] = '\0';
			return atol(pos);
		}
	}*/
	return 1500;
}

/**
* \brief function to reset configuration
	*
	* \param none
	* \return exit code, 0 if ok
	* \see ---
	*
*/
	int back2normal ()
{
	int iExist;
	printf ("Going back to normal configuration...");

/* clean the stored configuration */
	remove (SERVER_CONF_FILE);
/* kill all the cpu burn programs */
//TODO:cpufreq does not come back to normal configuration
	system ("killall -q burn");
	system ("killall -q cpulimd");
/* remove the malloc limitation and in the same time, all
	* other possible limitations */
//TODO:the file shouldn't be remove if others limitations exist

//OD 20070129 We don't use pam limits anymore
//remove (PAM_LIMITS);
//But we have to unlock memory
		if (dataMemoryLocked != NULL)
	{
		if (unlock_memory(dataMemoryLocked, lMemorySize) != 0)
			printf("Cannot release memory locked !\n");
		free(dataMemoryLocked);
		dataMemoryLocked = NULL;
	}
//TODO:change the /etc/cpulim.conf (restore the initial version)
/* delete the bandwitch and latency limitations */
	system ("tc qdisc del dev eth0 root &> /dev/null");
	system ("tc qdisc del dev eth0 ingress &> /dev/null");

/*	Whe have to reset cpu restriction	*/
/* 	Is there cpulim.old ?	*/

	iExist = access ( "/etc/cpulim.old", F_OK );
	if ( iExist == 0 )
	{
		system("cp /etc/cpulim.old /etc/cpulim.conf &> /dev/null");
	}
	return 0;
}

/**
* \brief function to generate seed
	*
	* \param r
	* \param seed_int
	* \return none
	* \see ---
	*
*/
	void generateSeed (gsl_rng * r, int seed_int)
{
/* generate the seed, defined or not by the user */
	if (seed_int == -1) {
		srand (time (NULL));
		gsl_rng_set (r, time (NULL));
	} else {
		srand (seed_int);
		gsl_rng_set (r, seed_int);
	}
}

/**
* \brief function to degrade cpu frequency
	*
	* \param user user to be affected
	* \param freq frequency to set
	* \return none
	* \see ---
	*
*/
void setCPU (char *user, int freq)
{
	int i;
	long nbcpu; 
	char file[SIZE_OF_STRING];
	FILE *in;
	struct passwd *user_struct;

	//printf("arg setCPU: user= %s, freq= %d",user,freq);
	nbcpu= (long)sysconf(_SC_NPROCESSORS_CONF);
	errno=0;
	user_struct=(struct passwd *) getpwnam(user);
	if(user_struct==NULL){
	    perror("user_struct is null for user, error:");
	    printf("User %s is missing",user);
	}
	
/*TODO: Test with cpufreq (verification)*/
	printf ("  -> frequency desired for cpu : %d MHz\n", freq);fflush(stdout);
	for (i = 0; i < nbcpu; ++i) {
		sprintf (file, "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_setspeed", i);
		if ((in = fopen (file, "w")) != NULL) {
			printf (" (with cpufreq)\n");
			fprintf (in, "%d", freq);
			fclose (in);
		} else {
			FILE *pp;
			char buf[SIZE_OF_STRING],buf2[SIZE_OF_STRING]="\0";
			char tmp[SIZE_OF_STRING];
			long percent;
			ldiv_t ratio;
			long int current,target;

			 printf ("(cpufreq not usable, use cpuburn)\n");

			if ((pp = popen ("cat /proc/cpuinfo |grep 'cycle frequency'", "r")) == NULL) {
				perror ("Error while reading the cpu configuration (/proc/cpuinfo)");
				exit (1);
			}
			fgets (buf, sizeof (buf), pp);
			//printf("buf ='%s', size=%d",buf,strlen(buf));fflush(stdout);
			pclose (pp);

			strcpy (buf, strchr (buf, ':') + 2);
			//printf("buf intermediaire='%s', size=%d\n",buf,strlen(buf));fflush(stdout);
			/*In the /proc alpha version, the cpu frequency is given in Hz with the word est. at the end */
			strncat(buf2,buf,strlen(buf)-6);
			/* freq is in MHz, buf2 in Hz */
			target=(long int)freq*1000000;
			current=atol(buf2);

			ratio=ldiv(target*100,current);
			percent=ratio.quot;
			printf ("     Thus %d%% of real cpu frequency\n",percent);

			/*	On sauvegarde cpulim.conf	*/
			system("cp /etc/cpulim.conf /etc/cpulim.old &> /dev/null");
			if ((in = fopen ("/etc/cpulim.conf", "w")) == NULL) {
			    perror ("Error while creating /etc/cpulim.conf");
			    exit (-1);
		}
			/* TODO: don't remove previous limitation (as for rlimit) and
			 * when the program is closed, remove the limitations that were
			 * set during the execution of the program (or save the file and
			 * restore it at the end of the program */
			if (user_struct != NULL){
			    printf("save /etc/cpulim.conf\n");
			    fprintf (in, "%d %d\n100 *\n", percent, user_struct->pw_uid);
			}
			/* TODO: just add the user limit and assume that the /etc/cpulim.conf
			 * already have "100 *" (in this case, /etc/cpulim.conf should always
			 * have at least "100 *")*/
			/* TODO: generate an error message for interactive mode if user
			 * doesn't exist */
			else{
			    printf("Pb with the user, default configuration in cpulim");
			    fprintf (in, "100 *\n");
			}
			fclose (in);

			/* Even if burn is used, /etc/cpulim.conf should contain the percent
			 * but in this case, all process will be limited */
			if (burn) {
			    printf("Burn CPU using 'burn %d'\n",100-percent);
			    sprintf (tmp, "burn %d", 100 - percent);
			    system (tmp);
			}else{
			    /*Else use cpulimd*/
			    printf("Burn CPU for processes of user %s using 'cpulimd' and 'cpulim %d %d'\n", user_struct->pw_name, 100-percent,user_struct->pw_uid);
			    system("killall -q cpulimd");
			    system("cpulimd");
			}
			return;
		}
	}
}

/**
* \brief function to reduce memory size
	*
	* \param user user to be affected
	* \param mem memory size
	* \return none
	* \see ---
	*
*/
	void setMem (char *user, int mem)
{
	int error;
	struct sysinfo s_info;
/*  FILE *in;
	char file[SIZE_OF_STRING], memString[SIZE_OF_STRING];

	sprintf (file, PAM_LIMITS);
	sprintf (memString, "%d", mem);
	printf ("  -> maxmalloc for user %s : %s kbytes\n", user, memString);
	in = fopen (file, "w+");
	fprintf (in, "%s\t-\tas\t%s\n", user, memString);
	fclose (in);
*/
//First we have to know the amount of RAM
	if (dataMemoryLocked != NULL)
	{
		if (unlock_memory(dataMemoryLocked, lMemorySize) != 0)
			printf("Cannot unlock memory\n");
		free(dataMemoryLocked);
		dataMemoryLocked = NULL;
	}

	error = sysinfo(&s_info);
//s_info.totalram in byte
//user wants only mem in byte
//so memory to lock : s_info.totalram - mem * 1024
	lMemorySize = s_info.totalram - mem * 1024 * 1024;
//printf("Memory wanted %u\n",lMemorySize);
	dataMemoryLocked = malloc(lMemorySize);
	if (dataMemoryLocked == NULL)
		printf("malloc error \n");
	if (lock_memory(dataMemoryLocked, lMemorySize) != 0)
		printf("Cannot lock memory \n");
}

/**
* \brief function to set local parameters such as CPU and memory
	*
	* \param doc XML document that contains parameters
	* \param node_islet XML Islet node to be affected
	* \param r to randomize value
	* \return none
	* \see setParam
	*
*/
void setLocalParam (xmlDocPtr doc, xmlNodePtr node_islet, gsl_rng * r)
{
	xmlChar *name, *islet_name, *seed, *cpu, *user, *mem;
	xmlXPathObjectPtr xpathObj;
	xmlXPathContextPtr xpathCtx = xmlXPathNewContext (doc);
	xmlNodePtr node;
	char arg1[SIZE_OF_STRING], arg2[SIZE_OF_STRING], buff[SIZE_OF_STRING];
	struct distrib *distrib;
	int seed_int = 0;

	islet_name = xmlGetProp (node_islet, "name");

	sprintf (buff, "//islet[@name='%s']//spec//seed", islet_name);
	xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
	seed =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
	xmlFree (xpathObj);

	node = node_islet->children->next->next->next;
	seed_int = atoi (seed);
	while (node) {
		name = xmlGetProp (node, "name");
		if (!strcmp (name, getip ()))
			break;
		xmlFree (name);
		seed_int++;
		node = node->next->next;
	}
	generateSeed (r, seed_int);

	sprintf (buff, "//islet[@name='%s']//spec//cpu//frequency", islet_name);
	xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
	cpu =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
	xmlFree (xpathObj);

//TODO: handle the case where there is several users limitation
	sprintf (buff, "//islet[@name='%s']//spec//resources//user", islet_name);
	xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
	user =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
	mem =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->next->next->
		xmlChildrenNode, 1);
	xmlFree (xpathObj);

	distrib = defDistrib (cpu);
	strcpy (arg1, distrib->arg1);
	strcpy (arg2, distrib->arg2);
	printf("going to set CPU\n");
	setCPU (user, dist (distrib->type, r, arg1, arg2));
	free (distrib);

	distrib = defDistrib (mem);
	strcpy (arg1, distrib->arg1);
	strcpy (arg2, distrib->arg2);
	setMem (user, dist (distrib->type, r, arg1, arg2));
	free (distrib);

	xmlFree (islet_name);
	xmlFree (seed);
	xmlFree (cpu);
	xmlFree (user);
	xmlFree (mem);
	xmlFree (xpathCtx);
}

/**
* \brief function setNetParam
	*
	* \param name Peer IP address affected by the rule
	* \param pbin Incoming bandwidth
	* \param bpout Outgoing bandwidth
	* \param lat Latency
	* \param i Handle of current queue (traffic control)
	* \param iParent Handle of parent queue (traffic control)
	* \return none
	* \see setNetParamInsideIslet, setNetParamInterIslet
	*
*/
	void setNetParam (char *name, double bpin, double bpout, double lat, int *i, int iParent)
{
	int j;
	char order[255];
	long lMtu = getMTU();
//First of all we have to get MTU value


	printf ("  -> -> with remote %d : %s\n",*i, name);
	printf ("        input flow bandwitch : %.3f Mbits/s\n", bpin);
	printf ("        output flow bandwitch : %.3f Mbits/s\n", bpout);
	printf ("        latency : %.2f ms\n", lat);

/**	For incoming traffic	**/
	sprintf (order,
		"tc filter add dev eth0 parent ffff: protocol ip prio 1 u32 match ip src %s police index %d rate %.0fkbit burst %.0fkbit mtu 1500 drop flowid :%d",
		name, iParent, bpin * 1000, bpin * 1000,(*i));
//  	sprintf (order,
//	   "tc filter add dev eth0 parent ffff: protocol ip prio 1 u32 match ip src %s police rate %.0fkbit burst %.0fkbit drop flowid :%d",
//	   name, bpin * 1000, bpin * 1000,++(*i));
	printf("%s\n",order);
	system (order);

/**	For outgoing traffic	**/
	sprintf (order,
		"tc class add dev eth0 parent 1:%d classid 1:%d htb rate %.0fkbit ceil %.0fkbit mtu %d",
		iParent,++(*i), bpout * 1000, bpout * 1000, lMtu);
	printf("%s\n",order);
	system (order);

	sprintf (order, "tc qdisc add dev eth0 parent 1:%d handle %d: prio", *i,
		*i);
	printf("%s\n",order);
	system (order);

	sprintf (order,
		"tc filter add dev eth0 parent 1: protocol ip prio 1 u32 match ip dst %s flowid 1:%d",
		name, *i);
	printf("%s\n",order);
	system (order);

//  sprintf (order, "tc qdisc add dev eth0 parent 1:%d handle %d5: pfifo limit 5",(int) i / 1000,
//	   i);
//  printf("%s\n",order);
//  system (order);

	for (j = 0; j < 3; j++) {
		sprintf (order,
			"tc qdisc add dev eth0 parent %d:%d handle %d: netem latency %.2fms",
			*i, j + 1, *i+j+1, lat);
		printf(order);
		printf("\n");
		system (order);
	}
	*i += j;
}

/**
* \brief function to set network parameters inside islet
	*
	* \param doc
	* \param node_islet points to the islet structure whose local machine belong to
	* \param r Variable to randomize value
	* \param i Handle of the current queue (traffic control)
	* \return none
	* \see setParam
	*
*/
	void setNetParamInsideIslet (xmlDocPtr doc, xmlNodePtr node_islet, gsl_rng * r, int *i)
{
	xmlChar *name, *bpin, *bpout, *lat, *islet_name;
	xmlNodePtr node;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	struct distrib *distrib;
	char buff[SIZE_OF_STRING], arg1[SIZE_OF_STRING], arg2[SIZE_OF_STRING];
	double bpincpy, bpoutcpy, latcpy;
	int iNbNode;
	int iParent;
	long lMtu = getMTU();
	FILE *pFile = NULL;
	char order[255];
	xpathCtx = xmlXPathNewContext (doc);
/*	Get the islet name	*/
	islet_name = xmlGetProp (node_islet, "name");
/*	Get the bpin distribution	*/
	sprintf (buff, "//islet[@name='%s']//spec//network//in_flow", islet_name);
	xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
	bpin =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
/*	Get the bpout distribution	*/
	bpout =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->next->next->
		xmlChildrenNode, 1);
/*	Get latency distribution	*/
	lat =
		xmlNodeListGetString (doc,
		xpathObj->nodesetval->nodeTab[0]->next->next->next->
		next->xmlChildrenNode, 1);
/*	calculate incoming bandwidth	*/
	xmlFree (xpathObj);
	distrib = defDistrib (bpin);
	strcpy (arg1, distrib->arg1);
	strcpy (arg2, distrib->arg2);
	bpincpy = dist (distrib->type, r, arg1, arg2);
	free (distrib);
/*	calculate outgoing traffic	*/
	distrib = defDistrib (bpout);
	strcpy (arg1, distrib->arg1);
	strcpy (arg2, distrib->arg2);
	bpoutcpy = dist (distrib->type, r, arg1, arg2);
	free (distrib);
/*	Calculate latency	*/
	distrib = defDistrib (lat);
	strcpy (arg1, distrib->arg1);
	strcpy (arg2, distrib->arg2);
	latcpy = dist (distrib->type, r, arg1, arg2);
	free (distrib);

	printf("Add class.\n");
	printf("Values used are: bpin=%f, bpout=%f, latenct=%f\n",bpincpy,bpoutcpy,latcpy);
	
	node = node_islet->children->next->next->next;
	iNbNode = 0;
	while (node) {
		iNbNode ++;
		node = node->next->next;
	}

	node = node_islet->children->next->next->next;
	(*i)++;
	pFile = fopen("/tmp/wrekaislet.tmp","w");
	iParent = *i;
	sprintf (order,
		"tc class add dev eth0 parent 1: classid 1:%d htb rate %.0fkbit ceil %.0fkbit mtu %d",
		*i, bpoutcpy * 1000, bpoutcpy * 1000,lMtu);
	printf("%s\n",order);
	system (order);

	printf("Add filters for the other computers of the current islet.\n");
	while (node) {
		name = xmlGetProp (node, "name");
		if (strcmp (name, getip ())) {
/*      distrib = defDistrib (bpin);
			strcpy (arg1, distrib->arg1);
			strcpy (arg2, distrib->arg2);
			bpincpy = dist (distrib->type, r, arg1, arg2);
			free (distrib);
			distrib = defDistrib (bpout);
			strcpy (arg1, distrib->arg1);
			strcpy (arg2, distrib->arg2);
			bpoutcpy = dist (distrib->type, r, arg1, arg2);
			free (distrib);
			distrib = defDistrib (lat);
			strcpy (arg1, distrib->arg1);
			strcpy (arg2, distrib->arg2);
			latcpy = dist (distrib->type, r, arg1, arg2);
			free (distrib);
*/      if (pFile != NULL)
			fprintf(pFile,"%s\n",name);
			setNetParam (name, bpincpy, bpoutcpy, latcpy, i, iParent);
		}
		xmlFree (name);
		node = node->next->next;
	}
	if (pFile != NULL)
		fclose(pFile);
	xmlFree (bpin);
	xmlFree (bpout);
	xmlFree (lat);
	xmlFree (islet_name);
	xmlFree (xpathCtx);
}

/**
* \brief function to set network parameters between two islets
	*
	* \param doc XML document
	* \param node_islet XML structure of the node affected by the parameters
	* \param r Variable to randomize value
	* \param i Handle of the queue (traffic control)
	* \return none
	* \see setParam
	*
*/
	void setNetParamInterIslet (xmlDocPtr doc, xmlNodePtr node_islet, gsl_rng * r, int *i)
{
	xmlChar *name, *bpin, *bpout, *lat, *seed, *islet1_name, *islet2_name;
	xmlNodePtr node, node_net;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj = NULL;
	struct distrib *distrib;
	char buff[SIZE_OF_STRING], arg1[SIZE_OF_STRING], arg2[SIZE_OF_STRING];
	double bpincpy, bpoutcpy, latcpy;
	int flag, iParent;
	FILE *pFile = NULL;
	long lMtu = getMTU();
	char order[255];

	xpathCtx = xmlXPathNewContext (doc);
	islet1_name = xmlGetProp (node_islet, "name");
	node_net = node_islet->parent->children->next;
	pFile = fopen("/tmp/wrekainter.tmp","w");
	while (node_net && !strcmp (node_net->name, "islet")) {
		islet2_name = xmlGetProp (node_net, "name");
		if (strcmp (islet1_name, islet2_name)) {
			sprintf (buff, "//network_spec[@from='%s' and @to='%s']", islet1_name,
				islet2_name);
			xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
			if (xpathObj->nodesetval->nodeNr == 0) {
				sprintf (buff, "//network_spec[@from='%s' and @to='%s']", islet2_name,
					islet1_name);
				xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
				flag = 1;
				} else
					flag = 0;
	/*	get outgoing bandwidthi distribution	*/
				bpout =
					xmlNodeListGetString (doc,
					xpathObj->nodesetval->nodeTab[0]->children->next->
					xmlChildrenNode, 1);
	/*	get incoming bandwidth distribution	*/
				bpin =
					xmlNodeListGetString (doc,
					xpathObj->nodesetval->nodeTab[0]->children->next->
					next->next->xmlChildrenNode, 1);
	/*	get latency distribution	*/
				lat =
					xmlNodeListGetString (doc,
					xpathObj->nodesetval->nodeTab[0]->children->next->
					next->next->next->next->xmlChildrenNode, 1);
	/*	get seed distribution	*/
				seed =
					xmlNodeListGetString (doc,
					xpathObj->nodesetval->nodeTab[0]->last->prev->
					xmlChildrenNode, 1);
				xmlFree (xpathObj);

				generateSeed (r, atoi (seed));
				xmlFree (seed);
	/*	calculate incoming bandwidth	*/
				distrib = defDistrib (bpin);
				strcpy (arg1, distrib->arg1);
				strcpy (arg2, distrib->arg2);
				bpincpy = dist (distrib->type, r, arg1, arg2);
				free (distrib);
	/*	calculate outgoing bandwidth	*/
				distrib = defDistrib (bpout);
				strcpy (arg1, distrib->arg1);
				strcpy (arg2, distrib->arg2);
				bpoutcpy = dist (distrib->type, r, arg1, arg2);
				free (distrib);
	/*	calculate latency	*/
				distrib = defDistrib (lat);
				strcpy (arg1, distrib->arg1);
				strcpy (arg2, distrib->arg2);
				latcpy = dist (distrib->type, r, arg1, arg2);
				free (distrib);

				(*i)++;
				iParent = *i;
				sprintf (order,
					"tc class add dev eth0 parent 1: classid 1:%d htb rate %.0fkbit ceil %.0fkbit mtu %d",
					*i, bpoutcpy * 1000, bpoutcpy * 1000,lMtu);
				printf("%s\n",order);
				system (order);

				node = node_net->children->next->next->next;
				while (node) {
					name = xmlGetProp (node, "name");

/*	distrib = defDistrib (bpin);
					strcpy (arg1, distrib->arg1);
					strcpy (arg2, distrib->arg2);
					bpincpy = dist (distrib->type, r, arg1, arg2);
					free (distrib);

					distrib = defDistrib (bpout);
					strcpy (arg1, distrib->arg1);
					strcpy (arg2, distrib->arg2);
					bpoutcpy = dist (distrib->type, r, arg1, arg2);
					free (distrib);

					distrib = defDistrib (lat);
					strcpy (arg1, distrib->arg1);
					strcpy (arg2, distrib->arg2);
					latcpy = dist (distrib->type, r, arg1, arg2);
					free (distrib);
*/    
					if (pFile != NULL)
						fprintf(pFile,"%s\n",name);
					if (flag) {
						setNetParam (name, bpincpy, bpoutcpy, latcpy, i,iParent);
					} else {
						setNetParam (name, bpoutcpy, bpincpy, latcpy, i,iParent);
					}

					xmlFree (name);
					node = node->next->next;
				}
				xmlFree (bpin);
				xmlFree (bpout);
				xmlFree (lat);
			}
			node_net = node_net->next->next;
			xmlFree (islet2_name);
		}
		if (pFile != NULL)
			fclose(pFile);

		xmlFree (islet1_name);
		xmlFree (xpathCtx);
	}

/**
	* \brief function reads xml document received and starts to set parameters
		*
		* \param none
		* \return none
		* \see setLocalParam
		* \see setNetParamInsideIslet
		* \see setNetParamInterIslet
		*
*/
void setParam ()
	{
		xmlNodePtr node_islet;
		xmlXPathContextPtr xpathCtx;
		xmlXPathObjectPtr xpathObj;
		char xmldoc[100] = SERVER_CONF_FILE, buff[SIZE_OF_STRING];
		xmlDocPtr doc = xmlParseFile (xmldoc);
		int i = 0, size;
		gsl_rng *r = gsl_rng_alloc (gsl_rng_default);

		if (doc == NULL) {
			perror ("Document not parsed successfully.");
			exit (-1);
		}
		if (xmlDocGetRootElement (doc) == NULL) {
			xmlFreeDoc (doc);
			perror ("Empty document");
			exit (-2);
		}
		xpathCtx = xmlXPathNewContext (doc);
		sprintf (buff, "//islet//machine[@name='%s']", getip ());
		xpathObj = xmlXPathEvalExpression (buff, xpathCtx);
		if (xpathObj == NULL)
		{
			printf("not found %s\n",buff);
		}
		node_islet = xpathObj->nodesetval->nodeTab[0]->parent;
		xmlFree (xpathObj);
/*	function to set local parameters such as memory size and cpu speed	*/
	//	printf("going to setLocalParam\n");fflush(stdout);
		setLocalParam (doc, node_islet, r);

//TODO:ONLY IP ADDRESSES ARE SUPPORTED
		xpathObj = xmlXPathEvalExpression ("//machine", xpathCtx);
		size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr - 1 : 0;
		xmlFree (xpathObj);

		printf ("  -> Number of specific network links : %d\n", size);
/*	Initialize queue for ingoing traffic	*/
		system ("tc qdisc add dev eth0 ingress handle ffff:");
		printf("tc qdisc add dev eth0 ingress handle ffff:\n");
/*	Initialize queue for outgoing traffic	*/
		system ("tc qdisc add dev eth0 root handle 1: htb");
		printf("tc qdisc add dev eth0 root handle 1: htb\n");
/*	Set network parameters for nodes inside islet	*/
		setNetParamInsideIslet (doc, node_islet, r, &i);
/*	Set network parameters for nodes inter islet	*/
		setNetParamInterIslet (doc, node_islet, r, &i);

/*	Free and close xml document	*/
		xmlFree (xpathCtx);
		xmlFreeDoc (doc);
		xmlCleanupParser ();
	}

/**
	* \brief function order receives orders from wrekavoc in xml format
		*
		* \param new_fd socket descriptor for the accepted socket
		* \return error code, 0 if none
		* \see setParam
		*
*/
int order (int new_fd)
	{
		char line[512];
		int bytes;
		int ouF;
		char xmldoc[100] = SERVER_CONF_DIR;
		int err;
//printf ("Acting!..\tpid=%d\tppid=%d\n", getpid (), getppid ());

		printf ("Receiving orders...\n\n");

/*	Creates a new xml document	*/
		if (opendir (xmldoc) == NULL)
			mkdir (xmldoc, 0755);

		xmldoc[0] = '\0';
		strcat (xmldoc, SERVER_CONF_FILE);
/*	and opens it	*/
		if ((ouF = open (xmldoc, O_RDWR | O_CREAT | O_TRUNC)) == -1) {
			printf("Error while manipuling the configuration file\n");
			exit (-1);
		}
/*	writes the content readed on the accepted socket into xml document	*/
		while ((bytes = read(new_fd, line, sizeof (line))) > 0) {
			printf("Line received (%d bytes): %s \n",bytes,line);
			err = write(ouF, line, (size_t)bytes);
			if (err==-1){
			    printf("erreur: %s", errno); 
			}else{
			    printf("Successfully writing %d bytes!\n",err);
			}
		}
		close (ouF);

		if(chmod (xmldoc, S_IRUSR | S_IWUSR)==-1)
		    printf("Error when chmod\n");
/*	Start setting new parameters	*/
		printf("just before setParam()"); fflush(stdout);
		setParam ();

		printf ("\n");

		return 0;
	}

/**
	* \brief function to check CPU configuration
		*
		* \param pitime Time of pim execution
		* \param new_fd handle to socket
		* \return none
		* \see ---
		*
		*/
void chckCPU (double pitime, int new_fd)
{
    FILE *pp;
    char buf[SIZE_OF_STRING];
    char buffer[SIZE_OF_STRING];
    struct timeval c1, c2;
    double newpitime, ratio;
    long real=0;
    struct passwd *user_struct;  

    //TODO:Add cpufreq support in check and multiple processor
    if ((pp = popen ("cat /proc/cpuinfo |grep 'cycle frequency'", "r")) == NULL) {
	perror ("Error while reading the cpu configuration (/proc/cpuinfo)");
    } else {
	fgets (buf, sizeof (buf), pp);
	pclose (pp);

	strcpy (buf, strchr (buf, ':') + 2);
	buf[strlen(buf)-6]='\0';
	//strncat(buf2,buf,strlen(buf)-6);
	//strcat(buf2,'\0');
	real=((long)atol(buf))/1000000;
	sprintf (buffer, "  -> current cpu frequency %d MHz (%s Hz)\n",real,buf);
	printf (buffer);
	write (new_fd, buffer, strlen (buffer));
    }

    //temp test time to compute 10000 decimals of pi
    CLOCK (c1);
    system ("pim2 10000 > /dev/null");
    CLOCK (c2);
    newpitime = CLOCK_DIFF (c2, c1);
    //printf ("initial pitime : %fsec\n", pitime);
    //printf ("new pitiome : %fsec\n", newpitime);

    ratio=100*pitime/newpitime;
    sprintf (buffer, "     %.1f %% of real cpu capability (%.1f MHz est.)\n",
	    ratio,ratio*real/100);
    printf (buffer);
    write (new_fd, buffer, strlen (buffer));

    FILE *pFile=NULL;
    pFile = fopen("/etc/cpulim.conf","r");

    //	if(pfile)
    //	    perror("cpulim.conf unreadable");
    int uid;
    uid_t euid;
    uid_t save_euid=geteuid();

    while(fscanf(pFile," %*d %d",&uid)!=0){

	euid=(uid_t)uid;
	seteuid(euid);
	//printf("current uid in the loop=%d\n",geteuid());

	CLOCK (c1);
	system ("pim2 10000 > /dev/null");
	CLOCK (c2);
	newpitime = CLOCK_DIFF (c2, c1);
	//	    printf ("initial pitime : %fsec\n", pitime);
	//	    printf ("new pitiome : %fsec\n", newpitime);

	ratio=100*pitime/newpitime;
	user_struct=(struct passwd*)getpwuid(euid);
	sprintf (buffer, "     %.1f %% of real cpu capability (%.1f MHz est.) for euid=%d (%s)\n",
		ratio,ratio*real/100,euid,user_struct->pw_name );
	printf (buffer);
	write (new_fd, buffer, strlen (buffer));
	seteuid(save_euid);
    }

}

/**
	* \brief function to check memory
		*
		* \param new_fd handle to socket
		* \return none
		* \see ---
		*
*/
void chckMem (int new_fd)
	{
		FILE *pFile;
		char buffer[SIZE_OF_STRING];
		unsigned long temp;
		void *test;

		pFile = fopen (PAM_LIMITS, "r");
		if (pFile == NULL) {
			printf ("  wrekavoc is not yet configured (file %s does not exist)\n",
				PAM_LIMITS);
		} else {
			char user[SIZE_OF_STRING];
	//TODO:This is correct ONLY if PAM_LIMITS contains only the
	//malloc limitation
			fscanf (pFile, "%s", user);

	/* test the maximum amout of memory that can be allocated */
	//this might be wrong because the result are stupid
	//TODO:Improved this memory limitation
	//TODO:Improves the test so that this is the user that do the test
	//(else it will be always root)
			temp = 10000;
			while ((test = calloc (temp, 1)) != NULL) {
				free (test);
				temp *= 1.1;		//accuracy = 10% (namely (1.1-1)*100)
			}
			sprintf (buffer, "  -> maxmalloc for user %s : %lu Mbyte\n", user,
				temp / 1024 / 1024);
			printf (buffer);
			write (new_fd, buffer, strlen (buffer));
			fclose (pFile);
		}
	}

/**
	* \brief function to check network link
		*
		* \param new_fd handle to socket
		* \return none
		* \see ---
		*
*/
		void chckNetLink (int new_fd)
	{
		struct timeval begin, end;
		size_t res, sent_size, buff_size, get_buff_size;
		double time, speed, time3, time2 = 10000000.0;
		int i, j, k, socket, size;
		char latbuffer[1], data_buffer[TCP_TIMING_SIZE], buffer[SIZE_OF_STRING];
		char xmldoc[100] = SERVER_CONF_FILE;
		xmlDocPtr doc;
		xmlXPathContextPtr xpathCtx;
		xmlXPathObjectPtr xpathObj;
		xmlNodeSetPtr nodes;
		xmlChar *name;

		doc = xmlParseFile (xmldoc);
		if (doc == NULL) {
			fprintf (stderr, "Document not parsed successfully.\n");
			return;
		}

		if (xmlDocGetRootElement (doc) == NULL) {
			fprintf (stderr, "Empty document\n");
			xmlFreeDoc (doc);
			return;
		}

		xpathCtx = xmlXPathNewContext (doc);
		xpathObj = xmlXPathEvalExpression ("//machine", xpathCtx);
		nodes = xpathObj->nodesetval;
		size = (nodes) ? nodes->nodeNr : 0;

		sprintf (buffer, "  -> Number of specific network links : %d\n",
			size ? size - 1 : 0);
		printf (buffer);
		write (new_fd, buffer, strlen (buffer));

		for (k = 0; k < size; ++k) {
			name = xmlGetNsProp (nodes->nodeTab[k], "name", NULL);
			if (strcmp (name, getip ())) {

			    /*BUG: Zero the latency values*/
			    time2=time3=10000000.0;
			    time=0.0;
			    sprintf (buffer, "  -> -> with remote : %s\n", name);
				printf (buffer);
				write (new_fd, buffer, strlen (buffer));

	/* setting new socket options */
				socket = startClientSocket (PORT_LATENCY, name);
				buff_size = TCP_BUFFER_SIZE;
				setsockopt (socket, SOL_SOCKET, SO_SNDBUF, (char *) &buff_size,
					sizeof (buff_size));

	/* time a short connection */
				sent_size = 0;
				CLOCK (begin);
				res =
					raw_send_in_pieces (socket, data_buffer, TCP_TIMING_SIZE, &sent_size,
					TCP_TIMING_SIZE / TCP_PIECE_SIZE, TCP_PIECE_SIZE);
				//res=write(socket,data_buffer,sizeof(data_buffer));
				read(socket,latbuffer,1);
				CLOCK (end);
				time = CLOCK_DIFF (end, begin);

				
	/* Add some tcp overhead */
				int nb_packets = (TCP_PIECE_SIZE/1480)*(res/TCP_PIECE_SIZE); /* Add overhead bytes. Each packet sent */
				res = res + nb_packets*48;


	/* setting saved socket options */
				setsockopt (socket, SOL_SOCKET, SO_SNDBUF, &get_buff_size,
					sizeof (get_buff_size));

	/* evaluate the speed of the network */
				speed = ((double) res) / (time * 1000000);	/*in Mbyte/s */
				sprintf (buffer, "        sending speed : %.3f Mbits/s (%.3f MBytes/s)(test size = %d bytes in %f s)\n", speed*8,speed,res,time);
				printf (buffer);
				write (new_fd, buffer, strlen (buffer));

	//printf ("\nTiming latency with read/write system call\n");
	//TODO:Reduce the number of ping test (sometime, a bit slow)
				for (j = 0; j < 10; j++) {
					CLOCK (begin);
					for (i = 0; i < 10; i++) {
						write (socket, latbuffer, 1);
						read (socket, latbuffer, 1);
					}
					CLOCK (end);
					time3 = CLOCK_DIFF (end, begin);
	//printf("\nMean duration of 1 byte ping pong : %.3f ms\n",time3*1000/50);
					if (time2 > time3)
						time2 = time3;
				}
				/*latency = time2*1000(to get ms) / 10 (10 ping pong) /2 (rtt=2*latency)*/
				sprintf (buffer, "        latency : %.3f ms\n", time2 * 50);
				printf (buffer);
				write (new_fd, buffer, strlen (buffer));
			}
			xmlFree (name);
		}
		close (socket);
		xmlFree (xpathObj);
		xmlFree (xpathCtx);
		xmlFreeDoc (doc);
		xmlCleanupParser ();
	}

/**
	* \brief function to check node configuration
		*
		* \param pitime time of pim2 execution
		* \param new_fd handle to socket
		* \return none
		* \see ---
		*
*/
		int check (double pitime, int new_fd)
	{
		char buf[MAXDATASIZE];
		printf ("Checking parameters:\n");

		if (read (new_fd, buf, 255) < 0)
			perror ("ERROR reading from socket");
		if (!strcmp ("a", buf)) {
			chckCPU (pitime, new_fd);
			//chckMem (new_fd);
			chckNetLink (new_fd);
		}
		printf ("\n");
		return 0;
	}

/**
	* \brief function ???
		*
		* \param none
		* \return int
		* \see ---
		*
*/
		int waitLatency ()
	{
		size_t buff_size;
		int skt;
		char data_buffer[TCP_TIMING_SIZE];
		int sin_size;
		int new_fd;
		struct sockaddr_in their_addr;	//connector's address information
		int i, j;
		char buffer[1];

/* The process will fork when an connection is set
		* The child porcess will participate to the bandwitch and latency test
			* and then be killed
			* The parent process will continue to listen possible connection
			* and eventually fork again... */
			printf("waitLatency\n");
		skt = startServerSocket (PORT_LATENCY);
		while (1) {
			sin_size = sizeof (struct sockaddr_in);
			if ((new_fd =
			accept (skt, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
				perror ("Not acceptable socket");
				continue;
			}
			if (!fork ()) {
				close (skt);		//child doesn't need the listener
	/* setting new socket options */
				buff_size = TCP_BUFFER_SIZE;
				setsockopt (new_fd, SOL_SOCKET, SO_SNDBUF, (char *) &buff_size,
					sizeof (buff_size));
				my_read (new_fd, data_buffer, TCP_TIMING_SIZE, __LINE__, __FILE__);
	//res=raw_recv(new_fd,data_buffer,count,val,0,&s_buffer,&store_size);
				char confirmation[1];
				write(new_fd,confirmation,1);
	/* Receiver side */
				for (j = 0; j < 10; j++) {
					for (i = 0; i < 10; i++) {
						read (new_fd, buffer, 1);
						write (new_fd, buffer, 1);
					}
				}
				close (new_fd);
				return 0;
			}
			close (new_fd);
		}
		return 0;
	}

/**
	* \brief Main function of wrekavocd
		*
		* \param argc Nomber of arguments
		* \param argv Pointer to an array pointer that contains arguments
		* \return int Exit code of the process
		* \see ---
		*
*/
		int main (int argc, char **argv)
	{
		struct timeval c1, c2;
		double pitime;
		char tmp[16];
		struct sockaddr_in their_addr;	//connector's address information
		int sockfd, new_fd;
		int sin_size;
		char *name = *argv;
//struct sched_param max, cur;
//int sched;

		argv++;
		if (*argv == NULL)
			daemon (0, 0);
		else {
			if (!strcmp (*argv, "-i"))
				argv++;
			else
				daemon (0, 0);
			if (*argv != NULL && !strcmp (*argv, "-burn")) {
				argv++;
				burn = 1;
			}
			if (*argv != NULL) {
				printf ("Usage: %s [-i] [-burn]\n", name);
				exit (-1);
			}
		}//(*argv == NULL)

		if (burn)
			system ("killall -q cpulimd");


/* initial test time to compute 10000 deimals of pi
		* need no other work in the same time */
//TODO:could have a better priority, just like the burn program
//TODO:change pim2 to be prioritized (only one time, it should not
//be always prioritized)
//TODO:test if the following solution is useful for the previous TODO
/*  sched_getparam (0, &cur);
			sched = sched_getscheduler (0);
		max.sched_priority = sched_get_priority_max (SCHED_FIFO);
		if (sched_setscheduler (0, SCHED_FIFO, &max) == -1) {
			perror ("Prioritizing the burn program did not succeed");
			}*/
			system ("killall -q burn");
			system("killall -q cpulimd");
			CLOCK (c1);
			system ("pim2 10000 > /dev/null");
			CLOCK (c2);
			pitime = CLOCK_DIFF (c2, c1);
			printf("pim2 10000 time is %f\n",pitime);fflush(stdout);
/*  if (sched_setscheduler (0, sched, &cur) == -1) {
			perror ("Coming back to normal priority failed");
			}*/
				if (fork ())
	/* The child process will be used for the bandwitch and
				* latency check (if it happens) */
			/*	exit(1);*/
				waitLatency ();
			else{
				system("rm /etc/cpulim.old &> /dev/null");
	/* The parent process will receive and execute the client order */
				sockfd = startServerSocket (PORT);
				printf ("Ready to receive orders\n\n");
				while (1) {
					sin_size = sizeof (struct sockaddr_in);
					if ((new_fd =
					accept (sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
						perror ("Not acceptable socket");
						continue;
					}
					read (new_fd, &tmp, 10);

	/* A new child process will execute the order
					* and then be killed while the parent process continue
						* to listen order from the client */
	//OD 2007/01/29 fork 
	/*if (!fork ())*/ {
		//TODO: wait the children from time to time (to avoid the
		//increasing number of zombies)
	//OD 2007/01/29 No fork so no close socket
	//close (sockfd);		//child doesn't need the listener
					if (strcmp (tmp, "order") == 0) {
						back2normal ();
	//TODO:when the previous test will be prioritized,
	//this will be unnecessary (to be removed)
						CLOCK (c1);
						system ("pim2 10000 > /dev/null");
						CLOCK (c2);
						pitime = CLOCK_DIFF (c2, c1);
						order (new_fd);
						} else if (strcmp (tmp, "check") == 0)
							check (pitime, new_fd);
						else if (strcmp (tmp, "normal") == 0)
							back2normal ();
						else if (strcmp (tmp, "quit") == 0) {
							back2normal ();
							system ("killall -q wrekavocd");
						} else {
							perror ("Bad command from the client\n");
						}
						shutdown (new_fd, 1);
	//return 0;
					}
	//close (new_fd);
				}
			}
			return 0;
		}
