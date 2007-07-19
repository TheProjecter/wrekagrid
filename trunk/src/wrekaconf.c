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

/**
 * Tool to create islet configuration
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

/**
 * \brief Structure for each islet
 *
 */
struct sIslet
{
	char strName[50];
	int iNbNode;
	char strBpIn[50];
	char strBpOut[50];
	int iSeed;
	char strCpu[50];
	char strLatency[50];
	char strUser[50];
	char strMemory[50];
	struct sIslet *pNext;
};

/**
 * \brief Function to count node in the file
 *
 * \param strFileNode : name of the file
 * \return Number of node
 *
 */
int countNodeInFile(char *strFileNode)
{
	FILE *pFile = NULL;
	char *strLine, *strOldLine;
	int iNbNode = 0;
	
	pFile = fopen(strFileNode,"r");
	
	if (pFile != NULL)
	{
		strLine = malloc(sizeof(char) * 256);
		strOldLine = malloc(sizeof(char) * 256);
		
		while (fscanf(pFile,"%s",strLine) > 0)
		{
			if (strlen(strLine) > 1)
			{
				if (strcmp(strLine,strOldLine) != 0)
				{
					iNbNode ++;
				}
				strcpy(strOldLine,strLine);
			}
		}
		free(strLine);
		free(strOldLine);
		fclose(pFile);
	}
	return iNbNode;
}

/**
 * \brief Function to free islet list
 *
 * \param pList List of islet
 * \return none
 *
 */
void freeIsletList(struct sIslet **pList)
{
	struct sIslet *pIslet, *pIsletTemp;
	pIslet = *pList;
	while (pIslet != NULL)
	{
		pIsletTemp = pIslet;
		pIslet = pIslet->pNext;
		free(pIsletTemp);
	}
}

/**
 * \brief Function to add an islet into the list
 *
 * \param pList List of islet
 * \return pointer to the new islet
 *
 */
struct sIslet * addIslet(struct sIslet **pList)
{
	struct sIslet *pIslet, *pNewIslet = NULL;
	pIslet = *pList;
	//It is the first element
	if (pIslet == NULL)
	{
		pNewIslet = malloc(sizeof(struct sIslet));
		if (pNewIslet != NULL)
		{
			pNewIslet->pNext = NULL;
			*pList = pNewIslet;
			return pNewIslet;
		}
		else
			return NULL;
	}
	else
	{
		//At the end of list
		while(pIslet->pNext != NULL)
		{
			pIslet = pIslet->pNext;
		}
		pNewIslet = malloc(sizeof(struct sIslet));
		if (pNewIslet != NULL)
		{
			pNewIslet->pNext = NULL;
			pIslet->pNext = pNewIslet;
			return pNewIslet;
		}
		else
			return NULL;
	}
	return NULL;
}

/**
 * \brief Function to write configuration file
 *
 * \param pList liste of islet
 * \param strFileConf name of file configuration
 * \return none
 *
 */
void writeConfiguration(struct sIslet **pList, char *strFileConf, char *strFileNode, char **strInter,int iNbInter)
{
	FILE *pFile = NULL, *pFileNode = NULL;
	int i;
	struct sIslet *pIslet = *pList;
	char strNode[256],strNodeIP[256], strOldNode[256];
	struct hostent *pHostent;

	
	pFile = fopen(strFileConf,"w");
	pFileNode = fopen(strFileNode,"r");
	if (pFile != NULL)
	{
		while(pIslet != NULL)
		{
			fprintf(pFile,"%s : ",pIslet->strName);
			for (i=0;i< pIslet->iNbNode;i++)
			{
				if (i > 0) fprintf(pFile,"-");
				if (fscanf(pFileNode,"%s",strNode) > 0)
				{
					if (strcmp(strNode, strOldNode) != 0)
					{
						//Search ip adress
						strcpy(strNodeIP,strNode);
						pHostent = gethostbyname(strNode);
						if (pHostent != NULL)
						{
							if (pHostent->h_addr_list[0] != NULL)
							{
								sprintf(strNodeIP,"%d.%d.%d.%d",(unsigned char)pHostent->h_addr_list[0][0],(unsigned char)pHostent->h_addr_list[0][1],(unsigned char)pHostent->h_addr_list[0][2],(unsigned char)pHostent->h_addr_list[0][3]);
							}
						}
						fprintf(pFile,"[%s-%s]",strNodeIP,strNodeIP);
						if (i == pIslet->iNbNode -1)
							fprintf(pFile," {\n");
					}
					else
					{
						i --;
					}
					strcpy(strOldNode,strNode);
				}
			}
			fprintf(pFile,"SEED : %d\n",pIslet->iSeed);
			fprintf(pFile,"CPU : %s\n",pIslet->strCpu);
			fprintf(pFile,"BPOUT : %s\n",pIslet->strBpOut);
			fprintf(pFile,"BPIN : %s\n",pIslet->strBpIn);
			fprintf(pFile,"LAT : %s\n",pIslet->strLatency);
			fprintf(pFile,"USER : %s\n",pIslet->strUser);
			fprintf(pFile,"MEM : %s\n",pIslet->strMemory);
			fprintf(pFile,"}\n\n");
			pIslet = pIslet->pNext;
		}
		for (i=0;i<iNbInter;i++)
		{
			fprintf(pFile,strInter[i]);
		}
	}
	if (pFile != NULL)
		fclose(pFile);
	if (pFileNode != NULL)
		fclose(pFileNode);
}

/**
 * \brief Function to calculate factorial
 *
 * \param n factorial to calculate
 * \return result of factorial n
 *
 */
int factorial(int n) 
{
	if ( n == 0) return 1;
	int result = n * factorial(n-1);
	return result;
}

/**
 * \brief Main function
 *
 * \param argc : Number of argument
 * \param argv : List of argument
 * \return 0 if OK
 *
 */
int main(int argc, char **argv)
{
	int iIndexArg = 1;
	char *strFileNode = NULL;
	char *strFileConf = NULL;
	int iNbIslet = 0;
	struct sIslet *pListIslet = NULL;
	struct sIslet *pTempIslet = NULL;
	int iNbNode = 0;
	int iNbNodeUsed= 0;
	int i,j;
	int iIsletNbNode;
	char strIsletBpIn[50];
	char strIsletBpOut[50];
	char strDefaultIsletBpIn[50] = "\0";
	char strDefaultIsletBpOut[50] = "\0";
	int iIsletSeed;
	int iDefaultSeed = -100;
	char strIsletCpu[50];
	char strIsletLatency[50];
	char strIsletUser[50],strDefaultUser[50] = "\0";
	char strIsletMemory[50];
	char strInterBpOut[50];
	char strInterBpIn[50];
	char strDefaultInterBpOut[50] = "\0";
	char strDefaultInterBpIn[50] = "\0";
	char strInterLatency[50];
	int iInterSeed;
	char strDefaultLatency[50] = "\0";
	char strDefaultMemory[50] = "\0";
	char strDefaultCpu[50] = "\0";
	int iNbInterIslet = 0;
	char **strInterIslets = NULL;
	int iIndexInter = 0;
	int iIsletsSize[5000];
	int iNbIsletsSize;
	char *strTemp;
	char *strPos1, *strPos2;
	
	//Check for arguments
	for (iIndexArg = 1; iIndexArg < argc;iIndexArg ++)
	{
		//Option -f
		if (strcmp(argv[iIndexArg],"-f") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strFileNode = malloc(sizeof(char) * strlen(argv[iIndexArg]));
				strcpy(strFileNode,argv[iIndexArg]);	
			}
		}
		//Option -o
		if (strcmp(argv[iIndexArg],"-o") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strFileConf = malloc(sizeof(char) * strlen(argv[iIndexArg]));
				strcpy(strFileConf,argv[iIndexArg]);	
			}
		}
		//Option -u
		if (strcmp(argv[iIndexArg],"-u") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strcpy(strDefaultUser,argv[iIndexArg]);
			}
		}
		//Option -s
		if (strcmp(argv[iIndexArg],"-s") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				iDefaultSeed = atoi(argv[iIndexArg]);
			}
		}
		//Option -il
		if (strcmp(argv[iIndexArg],"-il") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strcpy(strDefaultLatency,argv[iIndexArg]);
			}
		}
		//Option -l
		if (strcmp(argv[iIndexArg],"-l") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultLatency,"[%.2f;0]",atof(argv[iIndexArg]));
			}
		}
		//Option -im
		if (strcmp(argv[iIndexArg],"-im") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strcpy(strDefaultMemory,argv[iIndexArg]);
			}
		}
		//Option -m
		if (strcmp(argv[iIndexArg],"-m") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultMemory,"[%ld;0]",atol(argv[iIndexArg]));
			}
		}
		//Option -ic
		if (strcmp(argv[iIndexArg],"-ic") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				strcpy(strDefaultCpu,argv[iIndexArg]);
			}
		}
		//Option -c
		if (strcmp(argv[iIndexArg],"-c") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultCpu,"[%d;0]",atoi(argv[iIndexArg]));
			}
		}
		//Option --isletbp
		if (strcmp(argv[iIndexArg],"--isletbw") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultIsletBpIn,"[%.0f;0]",atof(argv[iIndexArg]));
				sprintf(strDefaultIsletBpOut,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}
		//Option --interbp
		if (strcmp(argv[iIndexArg],"--interbw") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultInterBpIn,"[%.0f;0]",atof(argv[iIndexArg]));
				sprintf(strDefaultInterBpOut,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}
		if (strcmp(argv[iIndexArg],"--isletbwin") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultIsletBpIn,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}
		//Option --interbp
		if (strcmp(argv[iIndexArg],"--interbwin") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultInterBpIn,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}	
		if (strcmp(argv[iIndexArg],"--isletbwout") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultIsletBpOut,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}
		//Option --interbp
		if (strcmp(argv[iIndexArg],"--interbwout") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				sprintf(strDefaultInterBpOut,"[%.0f;0]",atof(argv[iIndexArg]));
			}
		}
		if (strcmp(argv[iIndexArg],"--islets") == 0)
		{
			if (iIndexArg + 1 < argc)
			{
				iIndexArg ++;
				//we take all islets size
				strTemp = malloc(sizeof(char) * strlen(argv[iIndexArg]));
				strcpy(strTemp,argv[iIndexArg]);
				iNbIsletsSize = 0;
				strPos2 = strTemp;
				strPos1 = strTemp;
				do
				{
				       	strPos2	= strchr(strPos2,',');
					if (strPos2 != NULL)
					{
						strPos2[0] = '\0';
						strPos2 ++;
					}
					if (strlen(strPos1) > 0)
					{
						iIsletsSize[iNbIsletsSize] = atoi(strPos1);
						if (iIsletsSize[iNbIsletsSize] > 0)
							iNbIsletsSize ++;
					}
					strPos1 = strPos2;
				}while(strPos2 != NULL);
				free(strTemp);
			}
		}
		//Option -h
		if (strcmp(argv[iIndexArg],"--help") == 0 || strcmp(argv[iIndexArg],"-h") == 0) 
		{
			//Help synthax
			printf("Syntax : %s -f <file node>\tlike: filenode.txt\n",argv[0]);
		       	printf("\t-o <file conf>\tlike: file.conf\n");
		       	printf("\t-u <user>\tlike: g5k\n");
		        printf("\t-s <seed>\tlike: 1\n");
		        printf("\t-il <latency interval>\tlike: \"[100;0]\" for 100ms\n");
		        printf("\t-im <memory interval>\tlike: \"[2000;0]\" for 2000 Mo\n");
		        printf("\t-ic <CPU interval>\tlike: \"[2000;0]\" for 2000 MHz\n");
		        printf("\t-l <latency>\tlike: 100 for 100 ms\n");
		        printf("\t-m <memory>\tlike: 2000 for 2000 Mo\n");
		        printf("\t-c <CPU>\tlike: 2000 for 2000 MHz\n");
		        printf("\t--isletbw <Bandwidth in islet>\tlike: 15 for 15 Mbits/s\n");
		        printf("\t--interbw <Bandwidth inter islet>\tlike: 20 for 20 Mbits/s \n");
		        printf("\t--isletbwin <Incoming Bandwidth in islet>\tlike: 15 for 15 Mbits/s\n");
		        printf("\t--isletbwout <Outgoing Bandwidth in islet>\tlike: 15 for 15 Mbits/s\n");
		        printf("\t--interbwin <Incoming Bandwidth inter islet>\tlike: 20 for 20 Mbits/s\n");
		        printf("\t--interbwout <Outgoing Bandwidth inter islet>\tlike: 20 for 20 Mbits/s\n");
		        printf("\t--islets <Islets size>\tlike: 4,3,3\n");
			
			return 0;
		}
	}

	if (strFileNode == NULL)
	{
		printf("You must specified file containing node(s) with option -m !\n");
		return 1;
	}
	if (strFileConf == NULL)
	{
		strFileConf = malloc(sizeof(char) * strlen("file.conf"));
		strcpy(strFileConf,"file.conf");
	}

	//Count node in the file
	iNbNode = countNodeInFile(strFileNode);
	printf("You have %d node(s) in your file.\n",iNbNode);

	if (iNbIsletsSize > 0)
	{
		iNbIslet = iNbIsletsSize;
	}
	else
	{
		printf("How many Islet do you want ? ");
		scanf("%d",&iNbIslet);
	}

	printf("iNbIslet : %d\n",iNbIslet);
	
	if (iNbIslet > iNbNode)
	{
		printf("You don't have enough node !\n");
		return -1;
	}
	
	for (i=0;i<iNbIslet;i++)
	{
		printf("*********************************************\n");
		printf("*****           Islet%d                 *****\n",i);
		printf("*****           %d node(s) left         *****\n",iNbNode - iNbNodeUsed);
		printf("*********************************************\n\n");
		
		if (i < iNbIsletsSize)
		{
		       if (iIsletsSize[i] > 0 && iIsletsSize[i] <= (iNbNode - iNbNodeUsed))
		       {
			iIsletNbNode = iIsletsSize[i];
		       }
		       else
			       iIsletNbNode = 0;
		}
		else
			iIsletNbNode = 0;
		if (iIsletNbNode <= 0)
		{
			printf("How many node(s) int islet%d ? ",i);
			scanf("%d",&iIsletNbNode);
		}
		if (iDefaultSeed == -100)
		{
			printf("Enter seed value for islet%d ? ",i);
			scanf("%d",&iIsletSeed);
		}
		else
			iIsletSeed = iDefaultSeed;
		if (strlen(strDefaultCpu) <= 0)
		{
			printf("Enter CPU interval for islet%d ? ",i);
			scanf("%s",strIsletCpu);
		}
		else
			strcpy(strIsletCpu,strDefaultCpu);
		if (strlen(strDefaultIsletBpOut) <= 0)
		{
			printf("Enter BWOut interval for islet%d ? ",i);
			scanf("%s",strIsletBpOut);
		}
		else
			strcpy(strIsletBpOut,strDefaultIsletBpOut);
		if (strlen(strDefaultIsletBpIn) <= 0)
		{
			printf("Enter BWIn interval for islet%d ? ",i);
			scanf("%s",strIsletBpIn);
		}
		else
			strcpy(strIsletBpIn,strDefaultIsletBpIn);
		if (strlen(strDefaultLatency) <= 0)
		{
			printf("Enter Latency interval for islet%d ? ",i);
			scanf("%s",strIsletLatency);
		}
		else
			strcpy(strIsletLatency,strDefaultLatency);
		if (strlen(strDefaultUser) <= 0)
		{
			printf("Enter user for islet%d ? ",i);
			scanf("%s",strIsletUser);
		}
		else
			strcpy(strIsletUser,strDefaultUser);
		if (strlen(strDefaultMemory) <= 0)
		{
			printf("Enter memory interval for islet%d ? ",i);
			scanf("%s",strIsletMemory);
		}
		else
			strcpy(strIsletMemory,strDefaultMemory);
		iNbNodeUsed += iIsletNbNode;
		pTempIslet = addIslet(&pListIslet);
		if (pTempIslet != NULL)
		{
			pTempIslet->iNbNode = iIsletNbNode;
			pTempIslet->iSeed = iIsletSeed;
			sprintf(pTempIslet->strName,"islet%d",i);
			strcpy(pTempIslet->strCpu,strIsletCpu);
			strcpy(pTempIslet->strBpOut,strIsletBpOut);
			strcpy(pTempIslet->strBpIn,strIsletBpIn);
			strcpy(pTempIslet->strLatency,strIsletLatency);
			strcpy(pTempIslet->strUser,strIsletUser);
			strcpy(pTempIslet->strMemory,strIsletMemory);
		}
		else
		{
			printf("Cannot add new islet !\n");
			break;
		}
	}
	//Inter islet
	if (iNbIslet > 1)
	{
		printf("*********************************************\n");
		printf("******             Inter islets        ******\n");
		printf("*********************************************\n\n");
		//iNbInterIslet = (factorial(iNbIslet) / (2 * factorial(iNbIslet -2)));
		iNbInterIslet = iNbIslet * (iNbIslet - 1) / 2;
		strInterIslets = malloc(sizeof(char *) * iNbInterIslet);
		for (i=0;i<iNbIslet - 1;i++)
			for (j=i+1;j<iNbIslet;j++)
			{
				printf("******    Inter islet%d / islet%d    ******\n",i,j);
				if (strlen(strDefaultInterBpOut) <= 0)
				{
					printf("Enter BWOut interval for islet%d/islet%d ? ",i,j);
					scanf("%s",strInterBpOut);
				}
				else
					strcpy(strInterBpOut,strDefaultInterBpOut);
				if (strlen(strDefaultInterBpIn) <= 0)
				{
					printf("Enter BWIn interval for islet%d/islet%d ? ",i,j);
					scanf("%s",strInterBpIn);
				}
				else
					strcpy(strInterBpIn,strDefaultInterBpIn);
				if (strlen(strDefaultLatency) <= 0)
				{
					printf("Enter Latency interval for islet%d/islet%d ? ",i,j);
					scanf("%s",strInterLatency);
				}
				else
					strcpy(strInterLatency,strDefaultLatency);
				if (iDefaultSeed == -100)
				{
					printf("Enter seed for islet%d/islet%d ? ",i,j);
					scanf("%d",&iInterSeed);
				}
				else
					iInterSeed = iDefaultSeed;
				strInterIslets[iIndexInter] = malloc(sizeof(char) * 255);
				sprintf(strInterIslets[iIndexInter],"!INTER : [islet%d;islet%d] %s %s %s %d\n",i,j,strInterBpOut,strInterBpIn,strInterLatency,iInterSeed);
				iIndexInter ++;
			}
	}
	writeConfiguration(&pListIslet,strFileConf,strFileNode,strInterIslets,iNbInterIslet);
	
	//free all pointers
	if (strFileNode != NULL)
		free(strFileNode);
	freeIsletList(&pListIslet);
	if (strFileConf != NULL)
		free(strFileConf);
	for (i=0;i<iNbInterIslet;i++)
		free(strInterIslets[i]);
	if (strInterIslets != NULL)
		free(strInterIslets);
	return 0;
}
