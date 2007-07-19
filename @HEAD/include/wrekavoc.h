//#define MAXDATASIZE 100 // max number of bytes we can get at once
#define CLIENT_CONF_DIR "/.wrekavoc"	//directory where config is stored
#define CLIENT_CONF_FILE "/wrekavoc.conf"	//file where config is stored

//int buildXml(char * docname, char * xmldoc);
int checkXml (char *xmldoc);
int sendXml (char *xmldoc);
int killHost (char *xmldoc);
int Back2normalHost (char *xmldoc);
int checkHost (char *name, char *target);
