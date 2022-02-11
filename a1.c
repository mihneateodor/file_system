#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

void list(char * path, int recursive, char *name, int size){
	DIR *dir=NULL;
	struct dirent * entry=NULL;
	char fullPath[512];
	struct stat statbuf;

	dir=opendir(path);
	if(dir == NULL) {
		perror("ERROR\ncould not open directory\n");
		return ;
	}
	while( (entry=readdir(dir))!=NULL ){
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			if(recursive==0 && size==-1 && strcmp(name,"")==0){
				snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
				printf("%s\n",fullPath);
			}
			else if(recursive==1 && size==-1 && strcmp(name,"")==0){
				snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
				if(stat(fullPath,&statbuf)==0){
					printf("%s\n",fullPath);
					if(S_ISDIR(statbuf.st_mode)){
						list(fullPath,recursive,name,size);
					}
				}
			}
			else if(recursive==0 && size==-1 && strcmp(name,"")!=0){
				if(strncmp(name,entry->d_name,strlen(name))==0){
					snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
					printf("%s\n",fullPath);
				}
			}
			else if(recursive==1 && size==-1 && strcmp(name,"")!=0){
				if(strncmp(name,entry->d_name,strlen(name))==0){
					snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
					if(stat(fullPath,&statbuf)==0){
						printf("%s\n",fullPath);
						if(S_ISDIR(statbuf.st_mode)){
							list(fullPath,recursive,name,size);
						}
					}
				}
			}
			else if(recursive==0 && size!=-1 && strcmp(name,"")==0){
				snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
				if(stat(fullPath,&statbuf)==0){
					if(statbuf.st_size<size && S_ISREG(statbuf.st_mode))
						printf("%s\n",fullPath);
				}
			}
			else if(recursive==1 && size!=-1 && strcmp(name,"")==0){
				snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
				if(stat(fullPath,&statbuf)==0){
					if(statbuf.st_size<size && S_ISREG(statbuf.st_mode))
						printf("%s\n",fullPath);
					else if(S_ISDIR(statbuf.st_mode))
						list(fullPath,recursive,name,size);
				}
			}
		}
	}
	closedir(dir);
}

void parse(char * path){

	int fd=-1;
	fd=open(path,O_RDONLY);
	if(fd==-1){
		perror("ERROR\ncould not open file\n");
		return ;
	}

	//magic
	char magic[2];
	lseek(fd,-2,SEEK_END);
	for(int i=0;i<2;i++){
		if(read(fd,&magic[i],1)!=1){
			printf("ERROR\n");
			return;
		}
	}
	magic[2]=0;
	if(strcmp(magic,"Wo")!=0){
		printf("ERROR\nwrong magic\n");
		return;
	}

	//header_size
	unsigned char stringHeaderSize[2];
	int headerSize;
	lseek(fd,-4,SEEK_END);
	for(int i=1;i>=0;i--){
		read(fd,&stringHeaderSize[i],1);
	}
	headerSize=stringHeaderSize[0];
	headerSize<<=8;
	headerSize |=stringHeaderSize[1];

	//version
	unsigned char versionString;
	int version;
	lseek(fd,-headerSize,SEEK_END);
	read(fd,&versionString,1);
	version=versionString;
	if( version<77 || version>146){
		printf("ERROR\nwrong version\n");
		return;
	}

	//nr_sections
	unsigned char nr_sectionsString;
	int nr_sections;
	read(fd,&nr_sectionsString,1);
	nr_sections=nr_sectionsString;
	if( nr_sections<6 || nr_sections>19){
		printf("ERROR\nwrong sect_nr\n");
		return;
	}
	
	//sections
	char sect_name[15];
	int sect_type;
	int sect_offset;
	int sect_size;
	
	for(int i=0;i<nr_sections;i++){
		for(int j=0;j<15;j++){
			if(read(fd,&sect_name[j],1)!=1){
				printf("ERROR\n");
				return;
			}
		}
		read(fd,&sect_type,4);
		if(!(sect_type==19 || sect_type==61 || sect_type==20 ||
			sect_type==52 || sect_type==41 || sect_type==62 ||
			sect_type==97))
		{
			printf("ERROR\nwrong sect_types\n");
			return ;
		}
		read(fd,&sect_offset,4);
		read(fd,&sect_size,4);
	}

	//afisare
	printf("SUCCESS\n");
	printf("version=%d\n",version);
	printf("nr_sections=%d\n",nr_sections);
	int offset=nr_sections*(sizeof(sect_name)+sizeof(sect_type)+sizeof(sect_offset)+
		sizeof(sect_size))+sizeof(magic)+sizeof(stringHeaderSize);
	lseek(fd,-offset,SEEK_END);

	for(int i=0;i<nr_sections;i++){
		
		for(int j=0;j<15;j++){
			if(read(fd,&sect_name[j],1)!=1){
				printf("ERROR\n");
				return;
			}
		}
		read(fd,&sect_type,4);
		read(fd,&sect_offset,4);
		read(fd,&sect_size,4);
		printf("section%d: %s %d %d\n",i+1,sect_name,sect_type,sect_size);
	}

	close(fd);
}

void extract(char * path, int section, int line){
	int fd=-1;
	fd=open(path,O_RDONLY);
	if(fd==-1){
		perror("ERROR\ninvalid file\n");
		return ;
	}

	//magic
	char magic[2];
	lseek(fd,-2,SEEK_END);
	for(int i=0;i<2;i++){
		if(read(fd,&magic[i],1)!=1){
			printf("ERROR\n");
			return;
		}
	}
	magic[2]=0;
	if(strcmp(magic,"Wo")!=0){
		printf("ERROR\ninvalid file\n");
		return;
	}

	//header_size
	unsigned char stringHeaderSize[2];
	int headerSize;
	lseek(fd,-4,SEEK_END);
	for(int i=1;i>=0;i--){
		read(fd,&stringHeaderSize[i],1);
	}
	headerSize=stringHeaderSize[0];
	headerSize<<=8;
	headerSize |=stringHeaderSize[1];

	//version
	unsigned char versionString;
	int version;
	lseek(fd,-headerSize,SEEK_END);
	read(fd,&versionString,1);
	version=versionString;
	if( version<77 || version>146){
		printf("ERROR\ninvalid file\n");
		return;
	}

	//nr_sections
	unsigned char nr_sectionsString;
	int nr_sections;
	read(fd,&nr_sectionsString,1);
	nr_sections=nr_sectionsString;
	if( nr_sections<6 || nr_sections>19){
		printf("ERROR\ninvalid file\n");
		return;
	}
	
	//sections
	char sect_name[15];
	int sect_type;
	int sect_offset;
	int sect_size;
	
	for(int i=0;i<nr_sections;i++){
		for(int j=0;j<15;j++){
			if(read(fd,&sect_name[j],1)!=1){
				printf("ERROR\n");
				return;
			}
		}
		read(fd,&sect_type,4);
		if(!(sect_type==19 || sect_type==61 || sect_type==20 ||
			sect_type==52 || sect_type==41 || sect_type==62 ||
			sect_type==97))
		{
			printf("ERROR\ninvalid file\n");
			return ;
		}
		read(fd,&sect_offset,4);
		read(fd,&sect_size,4);
	}

	//extract propriu-zis
	if(section<0 || section>nr_sections){
		printf("ERROR\ninvalid section\n");
	}
	int offset=nr_sections*(sizeof(sect_name)+sizeof(sect_type)+sizeof(sect_offset)+
		sizeof(sect_size))+sizeof(magic)+sizeof(stringHeaderSize);
	lseek(fd,-offset,SEEK_END);
	for(int i=1;i<=nr_sections;i++){
		for(int j=0;j<15;j++){
			if(read(fd,&sect_name[j],1)!=1){
				printf("ERROR\n");
				return;
			}
		}
		read(fd,&sect_type,4);
		read(fd,&sect_offset,4);
		read(fd,&sect_size,4);
		if(section==i){
			break;
		}
	}

	lseek(fd,sect_offset,SEEK_SET);
	lseek(fd,sect_size-1,SEEK_CUR);
	char buffer[sect_size];
	char lineFinal[sect_size];
	int j=0;
	int lineVerif=1;

	for(int i=0;i<sect_size;i++){
		read(fd,&buffer[i],1);
		if(line==lineVerif){
			lineFinal[j]=buffer[i];
			if(lineFinal[j]==(char)0x0D && lineFinal[j-1]==(char)0x0A)
			{
				lineFinal[j+1]=0;
				break;
			}	
			j++;
		}
		else{
			if(buffer[i]==(char)0x0D && buffer[i-1]==(char)0x0A){
				lineVerif++;
			}
		}
		lseek(fd,-2,SEEK_CUR);
	}
	if(lineVerif>line){
		printf("ERROR\ninvalid line\n");
		return;
	}

	printf("SUCCESS\n");
	printf("%s\n",lineFinal);
	
	close(fd);
}

void success(int * ok){
	if(*ok==0){
		printf("SUCCESS\n");
		*ok=1;
	}
}
int ok=0;

void findall(char * path){
	DIR *dir=NULL;
	struct dirent * entry=NULL;
	char fullPath[512];
	struct stat statbuf;

	dir=opendir(path);
	if(dir == NULL) {
		printf("ERROR\ninvalid directory path\n");
		return ;
	}
	success(&ok);
	while( (entry=readdir(dir))!=NULL ){
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
			if(stat(fullPath,&statbuf)==0){
				if(S_ISREG(statbuf.st_mode)){
					int fd=-1;
					fd=open(fullPath,O_RDONLY);
					if(fd==-1){
						printf("ERROR\ninvalid directory path\n");
						return ;
					}

					char magic[2];
					lseek(fd,-2,SEEK_END);
					for(int i=0;i<2;i++){
						if(read(fd,&magic[i],1)!=1){
							printf("ERROR\n");
							return;
						}
					}
					magic[2]=0;
					if(strcmp(magic,"Wo")!=0){
						printf("ERROR\ninvalid directory path\n");
						return;
					}

					unsigned char stringHeaderSize[2];
					int headerSize;
					lseek(fd,-4,SEEK_END);
					for(int i=1;i>=0;i--){
						read(fd,&stringHeaderSize[i],1);
					}
					headerSize=stringHeaderSize[0];
					headerSize<<=8;
					headerSize |=stringHeaderSize[1];
					unsigned char versionString;
					int version;
					lseek(fd,-headerSize,SEEK_END);
					read(fd,&versionString,1);
					version=versionString;
					if( version<77 || version>146){
						printf("ERROR\ninvalid directory path\n");
						return;
					}
					unsigned char nr_sectionsString;
					int nr_sections;
					read(fd,&nr_sectionsString,1);
					nr_sections=nr_sectionsString;
					if( nr_sections<6 || nr_sections>19){
						printf("ERROR\ninvalid directory path\n");
						return;
					}

 					char pathFisier[512];
 					strcpy(pathFisier,fullPath);


					char sect_name[15];
					int sect_type;
					int sect_offset;
					int sect_size;

					for(int i=0;i<nr_sections;i++){
						for(int j=0;j<15;j++){
							if(read(fd,&sect_name[j],1)!=1){
								printf("ERROR\n");
								return;
							}
						}
						sect_name[15]=0;
						read(fd,&sect_type,4);
						if(!(sect_type==19 || sect_type==61 || sect_type==20 ||
							sect_type==52 || sect_type==41 || sect_type==62 ||
							sect_type==97))
						{
							printf("ERROR\ninvalid directory path\n");
							return ;
						}
						read(fd,&sect_offset,4);
						read(fd,&sect_size,4);
					}
					int section_header_size=15+4+4+4;
					int offset; 
					for(int i=0;i<nr_sections;i++){
						offset=nr_sections*section_header_size+sizeof(magic)+sizeof(stringHeaderSize)-i*section_header_size;
						lseek(fd,-offset,SEEK_END);
						for(int j=0;j<15;j++){
							if(read(fd,&sect_name[j],1)!=1){
								printf("ERROR\n");
								return;
							}
						}
						read(fd,&sect_type,4);
						read(fd,&sect_offset,4);
						read(fd,&sect_size,4);		
						//printf("%s %d %d %d\n",sect_name,sect_type,sect_offset,sect_size);
						lseek(fd,sect_offset,SEEK_SET);
						lseek(fd,sect_size-1,SEEK_CUR);
						int nr_linii=1;
						int ok=0;
						char buffer[sect_size];
						for(int j=0;j<sect_size && ok==0;j++){
							if(read(fd,&buffer[j],1)!=1){
								printf("ERROR\n");
								return;
							}
							if(buffer[j]==(char)0x0D && buffer[j-1]==(char)0x0A){
								nr_linii++;
							}
							if(nr_linii==13){
								ok=1;
								break;
							}
							lseek(fd,-2,SEEK_CUR);
						}
						if(ok==1){
							printf("%s\n",pathFisier);
							break;
						}
					}
					close(fd);
				}
				else if(S_ISDIR(statbuf.st_mode)){
					findall(fullPath);
				}
			}
		}
	}
}

int main(int argc, char **argv){
	if(argc >= 2){
		if(strcmp(argv[1], "variant") == 0){
			printf("18143\n");
		}
		else if(strcmp(argv[1], "list") == 0){
			int j=2;
			int recursive=0;
			int size=-1;
			char name[128];
			char path[512];
			while(j<argc){
				if( strncmp(argv[j],"path=",5) == 0 ){
					strcpy(path, argv[j]+5);
				}
				if( strcmp(argv[j],"recursive") == 0 ){
					recursive=1;
				}
				if( strncmp(argv[j],"size_smaller=",13) == 0 ){
					size=atoi( argv[j] + 13 );
				}
				if( strncmp(argv[j],"name_starts_with=", 17) == 0 ){
					strcpy(name, argv[j]+17);
				}
				j++;
			}
			struct stat statbuf;
			if( stat(path,&statbuf)!=0 || !(S_ISDIR(statbuf.st_mode)) ){
				printf("ERROR\ninvalid directory path\n");
			}
			else{
				printf("SUCCESS\n");
				list(path,recursive,name,size);
			}
		}
		else if(strcmp(argv[1],"parse")==0){
			char path[512];
			strcpy(path,argv[2]+5);
			parse(path);
		}
		else if(strcmp(argv[1],"extract")==0){
			char path[512];
			int section;
			int line;
			strcpy(path,argv[2]+5);
			section=atoi(argv[3]+8);
			line=atoi(argv[4]+5);
			extract(path,section,line);
		}
		else if(strcmp(argv[1],"findall")==0){
			char path[512];
			strcpy(path,argv[2]+5);

			//findall(path);
		}
	}
	return 0;
}

