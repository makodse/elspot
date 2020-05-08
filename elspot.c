/*elspot.c använder libcurl för att hämta elpriser från nordpools ftp.*/
/*v0.1 - använder libcurl, allt är hårdkodat skriver till influx*/
/*on debian: sudo apt install libcurl4-nss-dev */
/*gcc -o elspot elspot.c -lcurl*/
/* fulkodat lite för att hämta spotpriset till influx... 
användning, gärna med script: 
-hämta spotprice.sdv från nordpool
-anropa programmet som skickar kommande dygns värden till influx (gör detta efet 16 när de publiceras) "./elspot spotpice.sdv"*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>
#define MAXCHAR 1000
int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    char str[MAXCHAR];
    char my_row[MAXCHAR];
    char* filename = argv[1];
    char *ret;
    char *zone="SE4";
    char *currency="SEK";
    int result_row=0;
    char *found;
    const char *divider=";";
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Could not open file %s",(argv[1]));
        return 1;
    }
    while (fgets(str, MAXCHAR, fp) != NULL){
	/*check zone*/
    	if((strstr(str,zone)) != NULL)
    	{
		/*check currency*/
		if((strstr(str,currency)) != NULL)
    		{
			strcpy(my_row,str);
			/*add 1 to result, if correct only 1 match is found*/
			result_row=result_row+1;
		}
    	}
    }
    fclose(fp);
	if(result_row==1){
	/*parsing the result*/
		printf("%s", my_row);
        char b[40][12];
        int rad=0;
        int y=0;
        int i;
        for(i=0; i<strlen(my_row); i++)
        {
                if(my_row[i]==';'){
                        b[rad][y]='\0';
                        rad++;
                        y=0;
                }else{
                        b[rad][y]=my_row[i];
                        y++;
                }
        }
        /* justera sista raden och antal eftersom sista posten inte innehåller ;*/
        b[rad][y]='\0';
        rad++;
	/**/
	/*kopiera datumet*/
	char file_date[12];
	strcpy(file_date, b[5]);
	printf("file date: %s\n", file_date);
	int founddot=0;
	char found_day[5], found_month[5], found_year[5];
	int foundcount=0;
	for(int t=0; t<11; t++){
		if(founddot==0){
			/*day*/
			if(file_date[t]=='.'){
				found_day[foundcount]='\0';
				foundcount=0;
				founddot=1;
			}else{
				found_day[foundcount]=file_date[t];
				foundcount++;
			}
		}
		else if(founddot==1){
			 /*month*/
                        if(file_date[t]=='.'){
                                found_month[foundcount]='\0';
                                foundcount=0;
				founddot=2;
                        }else{
                                found_month[foundcount]=file_date[t];
                                foundcount++;
                        }

		}
		else if(founddot==2){
			 /*year*/
                        if(file_date[t]=='\0'){
                                found_year[foundcount]='\0';
                                foundcount=0;
                        }else{
                                found_year[foundcount]=file_date[t];
                                foundcount++;
                        }

		}
	}
	printf("date to server %s-%s-%s\n", found_year, found_month, found_day);
  int ifound_day=atoi(found_day);
  int ifound_month=atoi(found_month);
  int ifound_year=atoi(found_year);
  //ifound_day--;
  ifound_month--;
  /*förbered data till server*/
	float hour_price[24];
	int count_array=8;
	for(int t=0; t<24; t++)
	{
		if(count_array==11)
			count_array=12;
		hour_price[t]=atof(b[count_array]);
		hour_price[t]=hour_price[t]/10;
		count_array++;
	}
  curl = curl_easy_init();
  char postthis[2000] =  "";

  for(int t=0; t<24; t++){
    printf("Hour: %d Price: %.1f\n",t, hour_price[t]);

    char temp[10];
	  char hour_push[4];
	  snprintf(hour_push, 3, "%d", t);

	  gcvt(hour_price[t],4, temp);
    if(t>0)
    {
      strcat(postthis, "\n");

    }
    strcat(postthis, "test,sensornamn=s1 kr=");
    strcat(postthis, temp);
    struct tm nar_da = { 0, 0, t, ifound_day, ifound_month, (ifound_year-1900)};
    time_t rawtime = mktime(&nar_da);
    char nar_da_c[20];
    sprintf(nar_da_c,"%ld", rawtime);
	  printf("t: %s\n", nar_da_c);
    strcat(postthis, " ");
    strcat(postthis, nar_da_c);

	}
  printf("%s\n", postthis);
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.3.220:8086/write?db=db_ahome&precision=s");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
	}
	/*check if result_row != 1 then something went wrong... log it? where?*/
    return 0;
}
