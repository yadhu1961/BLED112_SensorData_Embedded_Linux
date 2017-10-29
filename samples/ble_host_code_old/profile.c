#include "profile.h"


profiles profile =
{
    {"00431c4aa7a4428ba96dd92d43c8c7cf",
        {"f1b41cdedbf54acf8679ecb8b4dca6fe",8},
        {"f1b41cdddbf54acf8679ecb8b4dca6fe",11},
        {"f1b41cdcdbf54acf8679ecb8b4dca6fe",14},
        3
    },
    {"00431c4ab7a4428ba96dd92d43c8c7cf",
        {"f1b41cdfdbf54acf8679ecb8b4dca6fe",18},
        5
    },
    {"00431c4ac7a4428ba96dd92d43c8c7cf",
        {"07e6bf2747d440e8aedb52777321efa6",22},
        {"8c7c03bc1bc843e38267f5c0c0a6eac8",24},
        {"79e404424e81411eaeec8e0befc3d24e",26}
    }
};

bool profile_init(char *attribute_file,char *sensor_permission_file)
{

    /*First read the attribute file and fill profile structure with with handles
     then get the uuids of the characteristics using the handles, fill values of uuid's
     in the structure, and then read the service permission file and update the
     permission level of the sensors based on service UUID's.
     */
    FILE *fp;
    char *line;
    ssize_t len = 0,read = 0;
    int number_lines_read = 0;

    fp = fopen(attribute_file, "r");

    if (fp == NULL) {
        if(DEBUG)printf("Failed to open the attribute_file,will use the harcoded values\n");
        return false;
    }

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("0 file is empty\n");
        return false;
    }
    profile.gyro_sensor.x.handle = atoi(line+2);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only one line present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.gyro_sensor.y.handle = atoi(line+2);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only  two lines present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.gyro_sensor.z.handle = atoi(line+2);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only  three lines present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.position_sensor.position.handle = atoi(line+9);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only  four lines present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.control_service.key_id.handle = atoi(line+7);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only  five lines present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.control_service.xml_data.handle = atoi(line+9);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only six lines present in the attribute file hence return and use hardcoded values\n");
        return false;
    }
    profile.control_service.xml_control.handle = atoi(line+12);

    if(DEBUG)printf("updated values of the handles from attribute file\n");
    fclose(fp);

    //Now opening sensor permission file;
     fp = fopen(sensor_permission_file, "r");
     if (fp == NULL) {
        if(DEBUG)printf("Failed to open the sensor_permission_file,will use the harcoded values\n");
        return false;
    }

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("0 file is empty\n");
        return false;
    }
    profile.gyro_sensor.permission_level = atoi(line+read-2);

    read = getline(&line, &len, fp);
    if(read == -1)
    {
        if(DEBUG)printf("only one line present in the file, will use hardcoded values for remaining fields\n");
        return false;
    }
    profile.position_sensor.permission_level = atoi(line+read-2);

    if(DEBUG)printf("%s\n","Updated the profile with permission levels" );
    fclose(fp);
    return true;
}