#include <stdio.h>
#include "ezxml/ezxml.h"

int main()
{
	char xml_file_path[] = "../configuration_files/xml.xml";
	
	ezxml_t f1 = ezxml_parse_file(xml_file_path);
	const char *teamname;
	ezxml_t team;
	
	team = ezxml_child(f1, "Module");
	
	printf("Type %s",ezxml_attr(team,"Type"));
	printf("variant %s",ezxml_attr(team,"Variant"));
	
	//for (team = ezxml_child(f1, "Module"); team; team = team->next) {
    //teamname = ezxml_attr(team, "name");
    //for (driver = ezxml_child(team, "driver"); driver; driver = driver->next) {
        //printf("%s, %s: %s\n", ezxml_child(driver, "name")->txt, teamname,
               //ezxml_child(driver, "points")->txt);
		//}
	//}
	ezxml_free(f1);

}
