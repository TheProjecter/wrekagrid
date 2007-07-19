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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/signal.h>
#include <netdb.h>


#define	USER "basil"			/* User for connection to hosts and testings */

/**
 * \brief structure for every node
 *
 */
struct sNodeProperty
{
	char strName[255];
	char strIP[255];
	double dCpuFrequency;
	long lMemorySize;
	char strIslet[255];
	double dCpuFrequencyWreka;
	long lMemorySizeWreka;
	double dTpsMes;
	double dCycleMes;
	long lMemorySizeMes;
	long lMemoryFree;
	float fLatencyIsletWreka;
	float fLatencyInterWreka;
	float fLatencyIsletMes;
	float fLatencyInterMes;
	int iBandWIsletWreka;
	int iBandWInterWreka;
	int iBandWIsletMes;
	int iBandWInterMes;
	struct sNodeProperty *sNext;
	struct sNodePorperty *sPrevious;
};

/**
 * \brief function to draw a sort of scrollbar in text mode in a thread
 * 
 * \param param : not used
 * \return void *, thread return value
 *
 */
void *scrollBar(void *param)
{
	while(1)
	{
		fprintf(stderr,"*");
		sleep(1);	
	}
	pthread_exit(NULL);
}

/**
 * \brief function to start scroll bar
 *
 * \param threadScroll points to a thread
 * \return none
 *
 */
void likeScrollBarStart(pthread_t **threadScroll)
{
	*threadScroll = malloc(sizeof(pthread_t));
	if (pthread_create(*threadScroll,NULL,scrollBar,NULL))
	{
		//erreur
	}
}

/**
 * \brief function to stop the scroll bar
 *
 * \param threadScroll points to a thread to exit
 * \return none
 *
 */
void likeScrollBarStop(pthread_t **threadScroll)
{
	if (*threadScroll != NULL)
	{
		pthread_cancel(**threadScroll );
		pthread_join(**threadScroll,NULL);
		free (*threadScroll);
	}
	printf("\r");
}

/**
 * \brief function to add a new node to a liste
 *
 * \param ppListe List in which to add a node
 * \param node Node to add
 * \return 0 if OK
 *
 */
int addNode(struct sNodeProperty **ppListe,char *node)
{
	struct sNodeProperty *sCurrent;
	char strIP[255];
	struct hostent *pHostent;

	if (*ppListe == NULL)
	{
		*ppListe = malloc(sizeof(struct sNodeProperty));
		if (*ppListe == NULL)
			return -1;
		(*ppListe)->sNext = NULL;
		(*ppListe)->sPrevious = NULL;
		strcpy((*ppListe)->strName,node);
		//Get ip adress
		strcpy(strIP,node);
		pHostent = gethostbyname(strIP);
                if (pHostent != NULL)
                {
                	if (pHostent->h_addr_list[0] != NULL)
			{
                        	sprintf(strIP,"%d.%d.%d.%d",(unsigned char)pHostent->h_addr_list[0][0],(unsigned char)pHostent->h_addr_list[0][1],(unsigned char)pHostent->h_addr_list[0][2],(unsigned char)pHostent->h_addr_list[0][3]);
			}
                }
		strcpy((*ppListe)->strIP,strIP);

	}
	else
	{
		sCurrent = *ppListe;
		while(sCurrent->sNext != NULL)
		{
			sCurrent = sCurrent->sNext;
		}
		//At the end of the list
		sCurrent->sNext = malloc(sizeof(struct sNodeProperty));
		if (sCurrent->sNext == NULL)
			return -1;
		((sCurrent->sNext)->sPrevious) = sCurrent;
		sCurrent = sCurrent->sNext;
		sCurrent->sNext = NULL;
		strcpy(sCurrent->strName,node);
		strcpy(strIP,node);
		pHostent = gethostbyname(strIP);
                if (pHostent != NULL)
                {
                	if (pHostent->h_addr_list[0] != NULL)
			{
                        	sprintf(strIP,"%d.%d.%d.%d",(unsigned char)pHostent->h_addr_list[0][0],(unsigned char)pHostent->h_addr_list[0][1],(unsigned char)pHostent->h_addr_list[0][2],(unsigned char)pHostent->h_addr_list[0][3]);
			}
                }
		strcpy(sCurrent->strIP,strIP);

	}
	return 0;
}

/**
 * \brief function to free all nodes in a list
 *
 * \param pListe list to free
 * \return 0 if OK
 *
 */
int delAllNodes(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrent, *pTemp;
	pCurrent = *pListe;
	if (pCurrent == NULL)
	{
		//Nothing to delete
	}
	else
	{
		do
		{
			pTemp = pCurrent;
			pCurrent = pCurrent->sNext;
			free(pCurrent);
		}while (pCurrent != NULL);
	}
	return 0;
}

/**
 * \brief function to create nodes from a file, and verify there is enough node
 *
 * \param machineFile name of the file which contains IP of nodes
 * \param pListe List to add node
 * \return 0 if OK
 *
 */
int checkMachineFile(char *machineFile, struct sNodeProperty **pListe)
{
	FILE *fFile;
	char *strBuff;
	int iNbNode = 0;
	
	fFile = fopen(machineFile,"r");
	if (fFile != NULL)
	{
		strBuff = malloc(256);
		while (fscanf(fFile, "%s",strBuff) > 0)
		{
			iNbNode ++;
			addNode(pListe,strBuff);
		}
		free(strBuff);
		fclose(fFile);
		if (iNbNode < 3)
		{
			printf("Must have at least 3 nodes !\n");
			return -1;
		}	       
		return 1;
	}
		
	printf("File %s does not exist !\n",machineFile);
	return 0;
}

/**
 * \brief function to read frequency from a temporary file
 *
 * \param fFrequency frequency read
 * \return 0 if OK
 *
 */
int litFrequenceCpu (double *fFrequency)
{
	const char * prefixe = "cycle frequency";   /* ALPHA: CPU info line beginning with this particular prefix*/
	FILE *fFile;
	char ligne[300+1];
	char *pos;
	int ok = 0;

	fFile =  fopen("/tmp/wrekavalid.tmp","r");
	if (!fFile)
		return 0;
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		if (!strncmp(ligne,prefixe,strlen(prefixe)))
		{
			pos = strrchr(ligne,':') +2;
			if (!pos) break;
			if (pos[strlen(pos) - 1] == '\n') pos[strlen(pos) - 1] = '\0';
			strncpy(ligne,pos,strlen(pos)-5);               /* ALPHA: line ending with "est."*/
			*fFrequency = atof(ligne);
			ok = 1;
			break;
		}
	}
	fclose (fFile);
	return ok;
}

/**
 * \brief function to read memory size from a temporary file
 *
 * \param lMemory Memory read
 * \return 0 if OK
 *
 */
int litTotalMemory (long *lMemory)
{
	const char *prefixe = "MemTotal";
	FILE *fFile;
	char ligne[300+1];
	char *pos;
	int ok = 0;
	fFile = fopen("/tmp/wrekavalid.tmp","r");
	if (!fFile)
		return 0;
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		if (!strncmp(ligne,prefixe,strlen(prefixe)))
		{
			pos = strrchr(ligne,':') + 2;
			if (!pos) break;
			if (pos[strlen(pos) - 1] == '\n') pos[strlen(pos) - 1] = '\0';
			strcpy(ligne,pos);
			*lMemory = atol(ligne);
			ok = 1;
			break;
		}
	}
	fclose(fFile);
	return ok;
}

int litFreeMemory (long *lMemory)
{
	const char *prefixe = "MemFree";
	FILE *fFile;
	char ligne[300+1];
	char *pos;
	int ok = 0;
	fFile = fopen("/tmp/wrekavalid.tmp","r");
	if (!fFile)
		return 0;
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		if (!strncmp(ligne,prefixe,strlen(prefixe)))
		{
			pos = strrchr(ligne,':') + 2;
			if (!pos) break;
			if (pos[strlen(pos) - 1] == '\n') pos[strlen(pos) - 1] = '\0';
			strcpy(ligne,pos);
			*lMemory = atol(ligne);
			ok = 1;
			break;
		}
	}
	fclose(fFile);
	return ok;
}
/**
 * \brief function to set/initialize all properties for a node
 *
 * \param pListe list that contains all nodes
 * \return 0 if OK
 *
 */
int checkNodeProperties(struct sNodeProperty **pListe)
{
        struct sNodeProperty *pCurrentNode;
	char strCommand[255];
	double dFrequency;
	long lMemory;
	long lMemoryFree;
	int iIndex = 0;
	char *user="basil";

	pCurrentNode = *pListe;
	if (pCurrentNode != NULL)
	{
		do
		{
			sprintf(strCommand,"ssh %s@%s \"cat /proc/cpuinfo | grep 'cycle frequency';cat /proc/meminfo | grep MemTotal;cat /proc/meminfo | grep MemFree\" > /tmp/wrekavalid.tmp",user, pCurrentNode->strName);
			system(strCommand);
			litFrequenceCpu(&dFrequency);
			litTotalMemory(&lMemory);
			litFreeMemory(&lMemoryFree);
			printf("Check node %s cpu frequency : %.2f, total memory : %ld\n",pCurrentNode->strName,dFrequency,lMemory);
			pCurrentNode->dCpuFrequency = dFrequency;
			pCurrentNode->lMemorySize = lMemory;
			pCurrentNode->lMemoryFree = lMemoryFree;
			switch(iIndex)
			{
				case 0 :
					strcpy(pCurrentNode->strIslet, "Islet1");
					pCurrentNode->dCpuFrequencyWreka = pCurrentNode->dCpuFrequency;
					pCurrentNode->lMemorySizeWreka = pCurrentNode->lMemorySize / 2;
					break;
				case 1 :
					strcpy(pCurrentNode->strIslet, "Islet2");
					pCurrentNode->dCpuFrequencyWreka = pCurrentNode->dCpuFrequency / 2;
					pCurrentNode->lMemorySizeWreka = pCurrentNode->lMemorySize / 3;
					break;
				case 2 :
					strcpy(pCurrentNode->strIslet, "Islet2");
					pCurrentNode->dCpuFrequencyWreka = pCurrentNode->dCpuFrequency / 2;
					pCurrentNode->lMemorySizeWreka = pCurrentNode->lMemorySize / 3;
					break;
			}
			pCurrentNode->fLatencyIsletWreka = 0;
			pCurrentNode->fLatencyInterWreka = 0;
			pCurrentNode->fLatencyIsletMes = 0;
			pCurrentNode->fLatencyInterMes = 0;
			pCurrentNode->iBandWIsletWreka = 0;
			pCurrentNode->iBandWInterWreka = 0;
			pCurrentNode->iBandWIsletMes = 0;
			pCurrentNode->iBandWInterMes = 0;
			iIndex ++;
			pCurrentNode = pCurrentNode->sNext;
		}while(pCurrentNode != NULL);
	}	
	return 0;
}

/**
 * \brief function to create a file configuration for wrekavoc (only to test latency)
 *
 * \param pListe list that contains all nodes
 * \param user user for wrekavoc
 * \return 0 if OK
 *
 */
int writeConfigurationFileLatence(struct sNodeProperty **pListe,char *user)
{
	struct sNodeProperty *pCurrentNode;
	FILE *fFile;
	int iIndex = 0;
	char strTempName[255];
	
	pCurrentNode  = *pListe;
	if (pCurrentNode != NULL)
	{
		fFile = fopen("/tmp/wrekavalid.conf","w");
		if (!fFile) 
		{
			printf("Cannot create file !\n");
			return 0; 
		}
		do
		{
			switch (iIndex)				
			{
				case 0 :
					pCurrentNode->fLatencyIsletWreka = 5;
					pCurrentNode->fLatencyInterWreka = 200;
					fprintf(fFile,"islet1 : [%s-%s] {\n",pCurrentNode->strIP,pCurrentNode->strIP);
					fprintf(fFile,"SEED : 1\n");
					fprintf(fFile,"CPU : [%d;0]\n",(int)(pCurrentNode->dCpuFrequency / 2 / 1000000));
					fprintf(fFile,"BPOUT : [10;0]\n");
					fprintf(fFile,"BPIN : [10;0]\n");
					fprintf(fFile,"LAT : [%.2f;0]\n",pCurrentNode->fLatencyIsletWreka);
					fprintf(fFile,"USER : %s\n",user);
					fprintf(fFile,"MEM : [%ld;0]\n",pCurrentNode->lMemorySizeWreka/1024);
					fprintf(fFile,"}\n");
					break;
				case 1 :
					pCurrentNode->fLatencyIsletWreka = 100;
					pCurrentNode->fLatencyInterWreka = 200;
					//remember node name
					strcpy(strTempName,pCurrentNode->strIP);
					break;
				case 2 :
					pCurrentNode->fLatencyIsletWreka = 100;
					pCurrentNode->fLatencyInterWreka = 200;
					fprintf(fFile,"islet2 : [%s-%s]-[%s-%s] {\n",pCurrentNode->strIP,pCurrentNode->strIP,strTempName,strTempName);
					fprintf(fFile,"SEED : 1\n");
					fprintf(fFile,"CPU : [%d;0]\n",(int)(pCurrentNode->dCpuFrequency * 2 / 3 /1000000));
					fprintf(fFile,"BPOUT : [5;0]\n");
					fprintf(fFile,"BPIN : [5;0]\n");
					fprintf(fFile,"LAT : [%.2f;0]\n",pCurrentNode->fLatencyIsletWreka);
					fprintf(fFile,"USER : %s\n",user);
					fprintf(fFile,"MEM : [%ld;0]\n",pCurrentNode->lMemorySizeWreka /1024);
					fprintf(fFile,"}\n");
					
					fprintf(fFile,"!INTER : [islet1;islet2] [15;0] [15;0] [%.2f;0] 1\n",pCurrentNode->fLatencyInterWreka);
					break;
			}
			iIndex ++;
			pCurrentNode = pCurrentNode->sNext;
		}while(pCurrentNode != NULL);
		//Inter islet
		fflush(fFile);
		fclose (fFile);
	}
	return 0;
}

/**
 * \brief function to create a file configuration for wrekavoc (to test bandwidth, memory, cpu)
 *
 * \param pListe list that contains all nodes
 * \param user user for wrekavoc
 * \return 0 if OK
 *
 */
int writeConfigurationFile(struct sNodeProperty **pListe, char *user)
{
	//We take only the three first elements
	struct sNodeProperty *pCurrentNode;
	FILE *fFile;
	int iIndex = 0;
	char strTempName[255];
	
	pCurrentNode  = *pListe;
	if (pCurrentNode != NULL)
	{
		fFile = fopen("/tmp/wrekavalid.conf","w");
		if (!fFile) 
		{
			printf("Cannot create file !\n");
			return 0; 
		}
		do
		{
			switch (iIndex)				
			{
				case 0 :
					pCurrentNode->iBandWIsletWreka = 10;
					pCurrentNode->iBandWInterWreka = 15;
					fprintf(fFile,"islet1 : [%s-%s] {\n",pCurrentNode->strIP,pCurrentNode->strIP);
					fprintf(fFile,"SEED : 1\n");
					fprintf(fFile,"CPU : [%d;0]\n",(int)(pCurrentNode->dCpuFrequency));
					fprintf(fFile,"BPOUT : [%d;0]\n",pCurrentNode->iBandWIsletWreka);
					fprintf(fFile,"BPIN : [%d;0]\n",pCurrentNode->iBandWIsletWreka);
					fprintf(fFile,"LAT : [0.05;0]\n");
					fprintf(fFile,"USER : %s\n",user);
					fprintf(fFile,"MEM : [%ld;0]\n",pCurrentNode->lMemorySizeWreka/1024);
					fprintf(fFile,"}\n");
					break;
				case 1 :
					pCurrentNode->iBandWIsletWreka = 5;
					pCurrentNode->iBandWInterWreka = 15;
					//remember node name
					strcpy(strTempName,pCurrentNode->strIP);
					break;
				case 2 :
					pCurrentNode->iBandWIsletWreka = 5;
					pCurrentNode->iBandWInterWreka = 15;

					fprintf(fFile,"islet2 : [%s-%s]-[%s-%s] {\n",pCurrentNode->strIP,pCurrentNode->strIP,strTempName,strTempName);
					fprintf(fFile,"SEED : 1\n");
					fprintf(fFile,"CPU : [%d;0]\n",(int)(pCurrentNode->dCpuFrequency/2));
					fprintf(fFile,"BPOUT : [%d;0]\n",pCurrentNode->iBandWIsletWreka);
					fprintf(fFile,"BPIN : [%d;0]\n",pCurrentNode->iBandWIsletWreka);
					fprintf(fFile,"LAT : [0.05;0]\n");
					fprintf(fFile,"USER : %s\n",user);
					fprintf(fFile,"MEM : [%ld;0]\n",pCurrentNode->lMemorySizeWreka/1024);
					fprintf(fFile,"}\n");
					fprintf(fFile,"!INTER : [islet1;islet2] [%d;0] [%d;0] [0.05;0] 1\n",pCurrentNode->iBandWInterWreka,pCurrentNode->iBandWInterWreka);
					
					break;
			}
			iIndex ++;
			pCurrentNode = pCurrentNode->sNext;
		}while(pCurrentNode != NULL);
		//Inter islet
		fflush(fFile);
		fclose (fFile);
	}
	return 0;
}

/**
 * \brief function to deploy wrekavoc on all nodes (not yet implemented)
 *
 * \param pListe list that contains all nodes
 * \return 0 if OK
 *
 */
int deployWrekavoc(struct sNodeProperty **pListe)
{
	return 0;
}

/**
 * \brief function to start wrekavoc deamons on all nodes
 *
 * \param pListe list that contains all nodes
 * \return 0 if OK
 *
 */
int startDaemon(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255];	
	pCurrentNode = *pListe;
	while (pCurrentNode != NULL)
	{
		//sprintf(strCommand,"ssh root@%s cpulimd",pCurrentNode->strName);
		//system(strCommand);	
		sprintf(strCommand,"ssh root@%s wrekavocd",pCurrentNode->strName);
		system(strCommand);	
		pCurrentNode = pCurrentNode->sNext;
		sleep(10);                              /* Wait at least 7s for pim2 10000 finishes */
	}
	return 0;
}

/**
 * \brief function to stop wrekavoc deamons on all nodes
 *
 * \param pListe list that contains all nodes
 * \return 0 if OK
 *
 */
int stopDaemon(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255];

	pCurrentNode = *pListe;
	while (pCurrentNode != NULL)
	{
		sprintf(strCommand,"ssh root@%s \"killall cpulimd\" > /dev/null",pCurrentNode->strName);
		system(strCommand);
		sprintf(strCommand,"ssh root@%s \"killall wrekavocd\" > /dev/null",pCurrentNode->strName);
		system(strCommand);
		pCurrentNode = pCurrentNode->sNext;
	}
	return 0;
}

/**
 * \brief function to start wrekavoc and send configuration to all nodes
 *
 * \param none
 * \return none
 *
 */
void startWrekavoc(void)
{
	char strCommand[255];

	sprintf(strCommand,"wrekavoc -r > /dev/null 2> /dev/null"); /* Suppose wrekavoc installed in path */ 
	system(strCommand);
	sleep(1);
	sprintf(strCommand,"wrekavoc /tmp/wrekavalid.conf ");
	system(strCommand);
	sleep(5);                               
}

/**
 * \brief function to read latency from a temporary file
 *
 * \param fLatency Latency read
 * \return 0 if OK
 *
 */
int litLatencyMes (float *fLatency)
{
	FILE *fFile;
	char ligne[300+1];
	char *pos, *pos2;
	int ok = 0;
	fFile = fopen("/tmp/wrekames.tmp","r");
	if (!fFile)
	{
		return 0;
	}
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		pos = strstr(ligne,"time=");
		if (!pos) break;
		pos2 = strrchr(pos,' ');
		if (!pos2) break;
		pos2[0] = '\0';
		strcpy(ligne,pos + strlen("time="));
		*fLatency = atof(ligne);
		ok = 1;
		break;
	}
	fclose(fFile);
	return ok;
}

/**
 * \brief function to test latency on every node with ping command
 *
 * \param pListe list that contains node
 * \return 0 if OK
 *
 */
int testLatence(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255];
	int iIndex = 0;
	float fLatency;
	char *user="basil";

	pCurrentNode = *pListe;
	while (pCurrentNode != NULL)
	{
		switch(iIndex)
		{
			case 0://Test inter islet
				if (pCurrentNode->sNext != NULL)
				{
					sprintf(strCommand,"ssh %s@%s \'ping %s -c 2\' | grep icmp_seq=2 > /tmp/wrekames.tmp",user,pCurrentNode->strName,(pCurrentNode->sNext)->strName); /* BUG: Need backticks, change user */
					system(strCommand);
					litLatencyMes(&fLatency);
					pCurrentNode->fLatencyInterMes = fLatency;
				}
				break;
			case 1://test dans islet
				if (pCurrentNode->sNext != NULL)
				{
					sprintf(strCommand,"ssh %s@%s \'ping %s -c 2\' | grep icmp_seq=2 > /tmp/wrekames.tmp",user,pCurrentNode->strName,(pCurrentNode->sNext)->strName);
					system(strCommand);
					litLatencyMes(&fLatency);
					pCurrentNode->fLatencyIsletMes = fLatency;
				}
				break;
		}
		iIndex ++;
		pCurrentNode = pCurrentNode->sNext;
	}
	return 0;
}

/**
 * \brief thread function to execute a shell command
 *
 * \param command Shell command to execute
 * \return thread exit value
 *
 */
void * threadCommand(void *command)
{
	char strCom[255];
	strcpy(strCom,(char *)command);
	system	(strCom);
	return NULL;
}

/**
 * \brief function to read bandwidth from a temporary file
 *
 * \param iBandwidth bandwidth read
 * \return 0 if OK
 *
 */
int litBandwidthMes (int *iBandwidth)
{
	FILE *fFile;
	char ligne[300+1];
	char *pos, *pos2;
	int ok = 0;
	fFile = fopen("/tmp/wrekames.tmp","r");
	if (!fFile)
	{
		return 0;
	}
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		pos = strstr(ligne,"MBytes ");
		if (!pos) break;
		pos2 = strrchr(pos,' ');
		if (!pos2) break;
		pos2[0] = '\0';
		strcpy(ligne,pos + strlen("Mbits "));
		*iBandwidth = atol(ligne);
		ok = 1;
		break;
	}
	fclose(fFile);
	return ok;
}

/**
 * \brief function to test bandwidth on every node with iperf command
 *
 * \param pListe list that contains node
 * \return 0 if OK
 *
 */
int testBp(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255], strCommand2[255];
	char strTemp[255];
	int iIndex = 0;
	int iBandwidth;
	pthread_t threadIperf;
	char *user="basil";
	
	pCurrentNode = *pListe;
	while(pCurrentNode != NULL)
	{
		switch(iIndex)
		{
			case 0:
				strcpy(strTemp,pCurrentNode->strName);
				break;
			case 1://On the 2nd node, we start iperf -s
				if (pCurrentNode->sNext != NULL && pCurrentNode->sPrevious != NULL)//3 nodes are available
				{
					sprintf(strCommand2,"ssh %s@%s \"iperf -s > /dev/null 2> /dev/null\"",user,pCurrentNode->strName);
					if (pthread_create(&threadIperf, NULL, threadCommand,strCommand2) == 0)
					{
						sleep(1);
						//start iperf on client
						sprintf(strCommand,"ssh %s@%s \"iperf -c %s -fm\" | grep Mbits/sec > /tmp/wrekames.tmp",user,strTemp, pCurrentNode->strName);
						system(strCommand);
						litBandwidthMes(&iBandwidth);
						pCurrentNode->iBandWInterMes = iBandwidth;
						sprintf(strCommand,"ssh %s@%s \"iperf -c %s -fm\" | grep Mbits/sec > /tmp/wrekames.tmp",user,(pCurrentNode->sNext)->strName, pCurrentNode->strName);
						system(strCommand);
						litBandwidthMes(&iBandwidth);
						pCurrentNode->iBandWIsletMes = iBandwidth;
						sleep(1);
						sprintf(strCommand,"ssh %s@%s \"killall iperf\" > /dev/null",user,pCurrentNode->strName); /* No need to be root to killall iperf ... and needs to enter another passwd! */
						system(strCommand);
						sleep(5);
						pthread_cancel(threadIperf );
						pthread_join(threadIperf,NULL);
					}
				}
				break;
		}
		iIndex ++;
		pCurrentNode = pCurrentNode->sNext;
	}
	return 0; 
}

/**
 * \brief function to read memory size from a temporary file
 *
 * \param lMemory memory size read
 * \return 0 if OK
 *
 */
int litMemoryMes (long *lMemory)
{
	const char *prefixe = "MemFree";
	FILE *fFile;
	char ligne[300+1];
	char *pos;
	int ok = 0;
	fFile = fopen("/tmp/wrekames.tmp","r");
	if (!fFile)
		return 0;
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		if (!strncmp(ligne,prefixe,strlen(prefixe)))
		{
			pos = strrchr(ligne,':') + 2;
			if (!pos) break;
			if (pos[strlen(pos) - 1] == '\n') pos[strlen(pos) - 1] = '\0';
			strcpy(ligne,pos);
			*lMemory = atol(ligne);
			ok = 1;
			break;
		}
	}
	fclose(fFile);
	return ok;
}

/**
 * \brief function to read times of a process from a temporary file
 *
 * \param dTps time of process
 * \param dCycle proportional to cycles used by process
 * \return 0 if OK
 *
 */
int litCpuMes (double *dTps, double *dCycle)
{
	FILE *fFile;
	char ligne[300+1], ligne2[300+1];
	char *pos, *pos2;
	int ok = 0;
	fFile = fopen("/tmp/wrekames.tmp","r");
	if (!fFile)
	{
		return 0;
	}
	while (!feof(fFile))
	{
		fgets(ligne,sizeof(ligne),fFile);
		strcpy(ligne2, ligne);
		pos = strstr(ligne2,"temps ");
		if (!pos) break;
		pos2 = strrchr(pos,' ');
		if (!pos2) break;
		pos2[0] = '\0';
		strcpy(ligne2,pos + strlen("temps "));
		*dTps = atof(ligne2);
		pos = strstr(ligne,"clock ");
		if (!pos) break;
		strcpy(ligne2,pos+strlen("clock "));
		*dCycle = atof(ligne2);
		ok = 1;
		break;
	}
	fclose(fFile);
	return ok;
}

/**
 * \brief function to test cpu frequency on every node with pimwreka
 *
 * \param pListe list that contains node
 * \return 0 if OK
 *
 */
int testCpu(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255];
	double dTps, dCyc;
	char *user="basil";

	pCurrentNode = *pListe;
	while (pCurrentNode != NULL)
	{
		sprintf(strCommand,"ssh %s@%s \"pimwreka 15000\" | grep temps > /tmp/wrekames.tmp",user,pCurrentNode->strName);
		system(strCommand);
		litCpuMes(&dTps, &dCyc);
		pCurrentNode->dTpsMes = dTps;
		pCurrentNode->dCycleMes = dCyc;
		pCurrentNode = pCurrentNode->sNext;
	}
	return 0;
}

/**
 * \brief function to test memory size on every node with top
 *
 * \param pListe list that contains node
 * \return 0 if OK
 *
 */
int testMem(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	char strCommand[255];
	long lMem;
	pCurrentNode = *pListe;
	char *user="basil";
	
	while (pCurrentNode != NULL)
	{
		sprintf(strCommand,"ssh %s@%s \"cat /proc/meminfo | grep MemFree\" > /tmp/wrekames.tmp",user,pCurrentNode->strName);
		system(strCommand);
		litMemoryMes(&lMem);
		pCurrentNode->lMemorySizeMes = lMem;
		pCurrentNode = pCurrentNode->sNext;
	}
	return 0;
}

/**
 * \brief function to report cpu results after the test
 *
 * \param pListe list that contains node
 * \return none
 *
 */
void rapportCpu(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	pCurrentNode = *pListe;
	float fRapport1, fRapport2;
	while (pCurrentNode != NULL)
	{
		fRapport1 = pCurrentNode->dTpsMes / pCurrentNode->dCycleMes;
		fRapport2 = pCurrentNode->dCpuFrequency / pCurrentNode->dCpuFrequencyWreka;

		printf("Node : %s, Cpu wanted : %.2f%%, measured : %.2f%%\n",pCurrentNode->strName,1 / fRapport2 * 100, 1 / fRapport1 * 100);
		//printf("Node : %s, Freq cpu : %.0f, wreka : %.0f, time : %.0f cycle : %.0f\n",pCurrentNode->strName,pCurrentNode->dCpuFrequency, pCurrentNode->dCpuFrequencyWreka, pCurrentNode->dTpsMes, pCurrentNode->dCycleMes);
//		fRapport1 = pCurrentNode->dTpsMes / pCurrentNode->dCycleMes;
//		fRapport2 = pCurrentNode->dCpuFrequency / pCurrentNode->dCpuFrequencyWreka;
		if ( fRapport1 > (fRapport2 - 0.3) && fRapport1 < (fRapport2 + 0.3))
			printf("\t=> OK\n");
		else
			printf("\t=> ERROR\n");
		pCurrentNode = pCurrentNode->sNext;
	}
}

/**
 * \brief function to report memory results after the test
 *
 * \param pListe list that contains node
 * \return none
 *
 */
void rapportMemory(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	double dWanted, dMeasured;

	pCurrentNode = *pListe;
	
	while (pCurrentNode != NULL)
	{
		dWanted = (double)pCurrentNode->lMemorySizeWreka * 100.0 / (double)pCurrentNode->lMemorySize;
		dMeasured = (double)pCurrentNode->lMemorySizeMes * 100.0 / (double)pCurrentNode->lMemoryFree;
		printf("Node : %s, Memory wanted : %.2f%%, measured : %.2f%%\n",pCurrentNode->strName, dWanted, dMeasured);
		//printf("Node : %s, total memory : %ld, wreka : %ld, meas : %ld\n",pCurrentNode->strName,pCurrentNode->lMemorySize, pCurrentNode->lMemorySizeWreka, pCurrentNode->lMemorySizeMes);
		if (dWanted - dMeasured < 5)
			printf("\t=> OK\n");
		else
			printf("\t=> ERROR\n");
		pCurrentNode = pCurrentNode->sNext;
	}
}

/**
 * \brief function to report latency results after the test
 *
 * \param pListe list that contains node
 * \return none
 *
 */
void rapportLatency(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	pCurrentNode = *pListe;
	
	while (pCurrentNode != NULL)
	{
		if (pCurrentNode->fLatencyIsletMes > 0 )
		{
			printf("Node : %s, Lat wreka : %.2fms, Lat Mes : %.2fms\n",pCurrentNode->strName,pCurrentNode->fLatencyIsletWreka, pCurrentNode->fLatencyIsletMes/2.0);
			if ((pCurrentNode->fLatencyIsletWreka *2.0 /pCurrentNode->fLatencyIsletMes) > 0.9)
				printf("\t=> OK\n");
			else
				printf("\t=> ERROR\n");
		}
		if (pCurrentNode->fLatencyInterMes > 0)
		{
			printf("Node : %s, Lat wreka : %.2fms, Lat Mes : %.2fms\n",pCurrentNode->strName,pCurrentNode->fLatencyInterWreka, pCurrentNode->fLatencyInterMes/2.0);
			if ((pCurrentNode->fLatencyInterWreka *2.0 /pCurrentNode->fLatencyInterMes) > 0.9)
				printf("\t=> OK\n");
			else
				printf("\t=> ERROR\n");
		}
		pCurrentNode = pCurrentNode->sNext;
	}
}

/**
 * \brief function to report bandwidth results after the test
 *
 * \param pListe list that contains node
 * \return none
 *
 */
void rapportBandwidth(struct sNodeProperty **pListe)
{
	struct sNodeProperty *pCurrentNode;
	pCurrentNode = *pListe;
	
	while (pCurrentNode != NULL)
	{
		if (pCurrentNode->iBandWIsletMes > 0 )
		{
			printf("Node : %s, BW Islet wreka : %dMbits/s, Measured : %dMbits/s\n",pCurrentNode->strName,pCurrentNode->iBandWIsletWreka, pCurrentNode->iBandWIsletMes);
			if (((float)pCurrentNode->iBandWIsletWreka / (float)pCurrentNode->iBandWIsletMes) > 0.9)
				printf("\t=> OK\n");
			else
				printf("\t=> ERROR\n");
		}
		if (pCurrentNode->iBandWInterMes > 0)
		{
			printf("Node : %s, BW Inter islet wreka : %dMbits/s, Measured : %dMbits/s\n",pCurrentNode->strName,pCurrentNode->iBandWInterWreka, pCurrentNode->iBandWInterMes);
			if (((float)pCurrentNode->iBandWInterWreka / (float)pCurrentNode->iBandWInterMes) > 0.9)
				printf("\t=> OK\n");
			else
				printf("\t=> ERROR\n");
		}
		pCurrentNode = pCurrentNode->sNext;
	}
}

/**
 * \brief Main function
 *
 * \param argc number of arguments
 * \param argv tab of arguments
 * \return 0 if OK
 *
 */
int main (int argc, char **argv)
{
	int iIndexArg;
	char *strFile = NULL;
	char *strUser = NULL;
	struct sNodeProperty *pNodeListe = NULL;
	//struct sNodeProperty *pCurrentNode = NULL;
	pthread_t *threadScrollBar = NULL;
	//Check arguments
	if (argc < 2)
	{
		printf("Correct syntax is : %s -m <machine_file> -u <user>\n",argv[0]);
		return 0;
	}
	for (iIndexArg = 1; iIndexArg < argc; iIndexArg ++)
	{
		if (strcmp(argv[iIndexArg],"-m") == 0)
		{
			//check there is another arg
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strFile = malloc(strlen(argv[iIndexArg]));
				strcpy(strFile,argv[iIndexArg]);	
			}
			else
			{
				printf("Argument in -m is missing !\n");
				return -1;
			}
		}
		if (strcmp(argv[iIndexArg],"-u") == 0)
		{
			//check if there is another arg
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strUser = malloc(strlen(argv[iIndexArg]));
				strcpy(strFile,argv[iIndexArg]);
			}
			else
			{
				printf("Argument in -u is missing !\n");
				return -1;
			}
		}
	}
	
	//I no user was specified 
	if (strUser == NULL)
	{
		strUser = malloc(4 * sizeof(char));
		strcpy(strUser,"basil");
		strUser[5] = '\0';
	}
	//Verify there is enough node in machine_file
	if (strFile != NULL)
	{
		if (!checkMachineFile(strFile, &pNodeListe))
		{
			return -2;
		}
		checkNodeProperties(&pNodeListe);
		//We can deploy wrekavoc on every node
		printf("\n*********************************************************\n");
		printf("Starting all daemons ...\n");
		printf("*********************************************************\n\n");
		deployWrekavoc(&pNodeListe);
		//We have to start wrekavocd and cpulimd on every node
		likeScrollBarStart(&threadScrollBar);
		startDaemon(&pNodeListe);
		likeScrollBarStop(&threadScrollBar);
		printf("\r*********************************************************\n");
		printf("Sending new configuration ...\n");
		printf("*********************************************************\n\n");
		//likeScrollBarStop(&threadScrollBar);
		//Once we have all properties, we can write the configuration file
		likeScrollBarStart(&threadScrollBar);
		writeConfigurationFileLatence(&pNodeListe, strUser);
		//then we start wrekavoc and we send config to all daemon
		startWrekavoc();
		likeScrollBarStop(&threadScrollBar);
		
		//So we can start to test wrekavoc
		//with first ping
		//testLatence
		printf("\r*********************************************************\n");
		printf("Testing latency ...\n");
		printf("*********************************************************\n\n");
		testLatence(&pNodeListe);
		rapportLatency(&pNodeListe);
		
		//then iperf
		printf("\r*********************************************************\n");
		printf("Testing bandwidth ...\n");
		printf("*********************************************************\n\n");
		//testBP
		likeScrollBarStart(&threadScrollBar);
		testBp(&pNodeListe);
		likeScrollBarStop(&threadScrollBar);
		rapportBandwidth(&pNodeListe);
	/*	
		//top
		//testMem
		printf("\n*********************************************************\n");
		printf("Testing memory ...\n");
		printf("*********************************************************\n\n");
		testMem(&pNodeListe);
		rapportMemory(&pNodeListe);
	*/
		//pim2	
		//testCpu
		printf("\n*********************************************************\n");
		printf("Testing cpu ...\n");
		printf("*********************************************************\n\n");
		likeScrollBarStart(&threadScrollBar);
		testCpu(&pNodeListe);
		likeScrollBarStop(&threadScrollBar);
		rapportCpu(&pNodeListe);
		//Stop all deamons
		printf("\n*********************************************************\n");
		printf("All daemons are stopped ...\n");
		printf("*********************************************************\n\n");
		stopDaemon(&pNodeListe);
		free(strFile);
		free (strUser);
	}
	delAllNodes(&pNodeListe);	
	return 0;
}
