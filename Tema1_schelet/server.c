#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001  


char checkParity(msg *mesaj)
{
	int i,j;
	char byte;
	for (i = 1; i < mesaj->len; i++)
		for (j = 7; j >= 0; j--)
			byte ^= ((mesaj->payload[i] >> j) & 1);
	return byte;
}

void computeParity(msg *mesaj)
{
	char byte = checkParity(mesaj);
	mesaj->payload[0] = byte;
	mesaj->len = strlen(mesaj->payload + 1) + 2;	
}

void computeParity2(msg *mesaj)
{
	char byte = checkParity(mesaj);
	mesaj->payload[0] = byte;
}

int verifyParity(msg *mesaj)
{
	char byte = checkParity(mesaj);
	if (mesaj->payload[0] == byte)
		return 1;
	else
		return 0;
}

int transferPack(msg *mesaj, int length)
{
	mesaj->len = length;
	send_message(mesaj);

	return recv_message(mesaj);	
}

void codare_hamming(msg *mesaj, char *s)
{
	int i,j;
	int p1,p2,p4,p8;
	int b[8];
	int cc = 0;
	for (i = 0; i < strlen(s); i++)
	{
		for (j = 0 ; j < 8 ; j++)
			b[j] = (s[i] >> j) & 1;

		p1 = (b[0] + b[1] + b[3] + b[4] + b[6]) % 2;
		p2 = (b[0] + b[2] + b[3] + b[5] + b[6]) % 2;
		p4 = (b[1] + b[2] + b[3] + b[7]) % 2;
		p8 = (b[4] + b[5] + b[6] + b[7]) % 2;
		
		if (p1 == 1)
			mesaj->payload[cc] |= 1 << 3;
		if (p2 == 1)				
			mesaj->payload[cc] |= 1 << 2;

		if (b[0] == 1)				
			mesaj->payload[cc] |= 1 << 1;
		if (p4 == 1)				
			mesaj->payload[cc] |= 1 << 0;
		if (b[1] == 1)				
			mesaj->payload[cc+1] |= 1 << 7;
		if (b[2] == 1)				
			mesaj->payload[cc+1] |= 1 << 6;
		if (b[3] == 1)				
			mesaj->payload[cc+1] |= 1 << 5;
		if (p8 == 1)				
			mesaj->payload[cc+1] |= 1 << 4;
		if (b[4] == 1)				
			mesaj->payload[cc+1] |= 1 << 3;
		if (b[5] == 1)				
			mesaj->payload[cc+1] |= 1 << 2;
		if (b[6] == 1)				
			mesaj->payload[cc] |= 1 << 1;
		if (b[7] == 1)				
			mesaj->payload[cc] |= 1 << 0;

		cc += 2;
	}
	mesaj->len = strlen(mesaj->payload);
}

void decodare_hamming(msg *m)
{
	int i,j;
	int p1,p2,p4,p8;
	int poz;

	for (i = 0 ; i < strlen(m->payload); i= i+2)
	{
		p1 = ((m->payload[i] >> 3) & 1) ^ ((m->payload[i] >> 1) & 1) ^ ((m->payload[i+1] >> 7) & 1) ^ ((m->payload[i+1] >> 5) & 1) ^ ((m->payload[i+1] >> 3) & 1) ^ ((m->payload[i+1] >> 1) & 1) ;
		
		p2 = ((m->payload[i] >> 2) & 1) ^ ((m->payload[i] >> 1) & 1) ^ ((m->payload[i+1] >> 6) & 1) ^ ((m->payload[i+1] >> 5) & 1) ^ ((m->payload[i+1] >> 2) & 1) ^ ((m->payload[i+1] >> 1) & 1) ;
	
		p4 = ((m->payload[i] >> 0) & 1) ^ ((m->payload[i+1] >> 7) & 1) ^ ((m->payload[i+1] >> 6) & 1) ^ ((m->payload[i+1] >> 5) & 1) ^ ((m->payload[i+1] >> 0) & 1) ;
	
		p8 = ((m->payload[i+1] >> 4) & 1) ^ ((m->payload[i+1] >> 3) & 1) ^ ((m->payload[i+1] >> 2) & 1) ^ ((m->payload[i+1] >> 1) & 1) ^ ((m->payload[i+1] >> 0) & 1) ;

		if ((p1 != 0) || (p2 != 0) || (p4 != 0) || (p8 != 0))
		{
			poz = p8 * 8 + p4 * 4 + p2 * 2 + p1;	

			if (poz <= 4)
				m->payload[i] ^= 1 << (4 - poz);
			else
				m->payload[i+1] ^= 1 << (12 - poz);

		}

		m->payload[i] &= ~(1 << 3);
		m->payload[i] &= ~(1 << 2);

		if (((m->payload[i] >> 1) & 1))
			m->payload[i] |= 1 << 7;

		if (((m->payload[i+1] >> 7) & 1))
			m->payload[i] |= 1 << 6;
		
		if (((m->payload[i+1] >> 6) & 1))
			m->payload[i] |= 1 << 5;

		if (((m->payload[i+1] >> 5) & 1))
			m->payload[i] |= 1 << 4;

		if (((m->payload[i+1] >> 3) & 1))
			m->payload[i] |= 1 << 3;

		if (((m->payload[i+1] >> 2) & 1))
			m->payload[i] |= 1 << 2;
		
		if (((m->payload[i+1] >> 1) & 1))
			m->payload[i] |= 1 << 1;
	
		if (((m->payload[i+1] >> 0) & 1))
			m->payload[i] |= 1 << 0;

		for (j = 7 ; j >= 0 ; j--)
			m->payload[i+1] &= ~(1 << j);
	}
	m->len = strlen(m->payload);
	
}


int listareDir(msg *mesaj, char *mod, char *comm)
{

 	msg t;
	int file_count = 0, res;	

	DIR *dirp;
	struct dirent *dp;
		dirp = opendir(comm);		
		while ((dp = readdir(dirp)) != NULL)
			file_count++;
		closedir(dirp);

		if (strcmp (mod,"parity") == 0)
		{
			do 
			{
				sprintf(t.payload + 1, "%d", file_count);
				computeParity(&t);
				send_message(&t);
				res = recv_message(&t);
			} 
			while (t.len == 5);
		} else
		{
			sprintf(t.payload,"%d",file_count);			
			res = transferPack(&t, strlen(t.payload) + 1);
		}	
		dirp = opendir(comm);	
 		while ((dp = readdir(dirp)) != NULL)
		{
			if (strcmp(mod,"parity") == 0)
			{
				do 
				{
					sprintf(t.payload + 1, "%s", dp->d_name);
				    computeParity(&t);
					send_message(&t);
					res = recv_message(&t);
				} 
				while (t.len == 5) ;
			} else
			{
				strcpy(t.payload, dp->d_name);
				res = transferPack(&t,strlen(t.payload) + 1);
			}
		}
		closedir(dirp);

	return recv_message(mesaj);
}

int changeDir( msg *mesaj, char *comm)
{
	chdir(comm);	
	return recv_message(mesaj);
}


int copyToClient(msg *mesaj, char *mod, char *comm)
{
	msg t;

	int cont, res;
	struct dirent *dp;
	DIR *dirp;
	dirp = opendir(".");

	while ((dp = readdir(dirp)) != NULL)
	{
		if (strcmp(dp->d_name,comm) == 0)
		{
		  	FILE *fp;
			fp = fopen(comm,"r");
			fseek(fp,0,SEEK_END);
			long size = ftell(fp);
			if (strcmp(mod,"parity") == 0)
			{
				do {
					sprintf(t.payload + 1,"%ld",size);
					t.len = strlen(t.payload + 1) + 1;
					computeParity2(&t);
					send_message(&t);
					res = recv_message(&t);

				   } while (t.len == 5);
			} else
			{
				sprintf(t.payload, "%ld", size);
				res = transferPack(&t,sizeof(size));
			} 
			fseek(fp,0,SEEK_SET);
			if (res > 0)
			{
				if (strcmp(mod,"parity") == 0)
				{	
					do{
						cont = fread(t.payload + 1, sizeof(char),1399,fp);
						t.len = cont + 1;
						computeParity2(&t);
						send_message(&t); 
						res = recv_message(&t);
					} while (t.len == 5);	
				} else
				{
					cont = fread(t.payload,sizeof(char),1400,fp);
					res = transferPack(&t, cont);
				}		
			}
			while (cont > 0)
			{
				if (res > 0)
				{
					if (strcmp(mod,"parity") == 0)
					{	
						do{
							cont = fread(t.payload + 1,sizeof(char),1399,fp);
							t.len = cont + 1;
							computeParity2(&t);
							send_message(&t);
							res = recv_message(&t);
						 } while (t.len == 5);	
					} else
					{
						cont = fread(t.payload,sizeof(char),1400,fp);
						res = transferPack(&t,cont);
					}		
				}
			}
			fclose(fp);
			
			break;	
		}	
	}
	return recv_message(mesaj);
}

int sentToServer(msg *mesaj, char *mod, char *comm)
{
	char *newf = malloc(sizeof(char));
	strcpy(newf,"new_");
	int size,i = 0, res;
	msg t;
	
	strcat(newf,comm);
	res = recv_message(&t);

	if (res > 0)
	{
		if (strcmp(mod,"parity") == 0)
		{
			while (verifyParity(&t) == 0)
			{
				strcpy(t.payload,"NACK");
				res = transferPack(&t, (strlen("NACK") + 1));
			}
			size = atoi(t.payload + 1);
			strcpy(t.payload,"ACK");
			res = transferPack(&t,strlen("ACK") + 1);
		} else
		{
			size = atoi(t.payload);
			strcpy(t.payload,"ACK");
			res = transferPack(&t,strlen("ACK"));
		}
	}
	FILE *fp;
	fp  = fopen(newf,"w");
	fseek(fp,0,SEEK_SET);
		
	while(i <= size)
	{
		if (res > 0)
		{
			if (strcmp(mod,"parity") == 0)
			{
				while (verifyParity(&t) == 0)
				{
					strcpy(t.payload,"NACK");
					res = transferPack(&t,(strlen("NACK") + 1));
				}
				fwrite(t.payload + 1,sizeof(char),t.len - 1,fp);
				strcpy(t.payload,"ACK");
				res = transferPack(&t,strlen("ACK") + 1);
			} else
			{
				fwrite(t.payload,sizeof(char),t.len,fp);
				strcpy(t.payload,"ACK");
				res = transferPack(&t,strlen("ACK"));
			}
		}
		i += 1400;
	}
	fclose(fp);	
	*mesaj = t;
	return res;
}

int main(int argc, char *argv[])
{
	msg r,t;
	int  res;
	char *mod, *point, *token, *aux;

	if (argc == 2)
		mod = argv[1];
	else
		mod = "none";

	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	res = recv_message(&r);

	while(1)
	{
		fflush(stdin);

		strcpy(t.payload,"ACK");

		if (strcmp(mod,"parity") == 0)
			t.len = strlen("ACK") + 1;
		else
			t.len = strlen("ACK");

		if (strcmp(mod,"parity") == 0)
		{
			char byte;
			byte  = checkParity(&r);

			if (r.payload[0] != byte)
			{
				strcpy(t.payload,"NACK");
				t.len = strlen("NACK") + 1;
				send_message(&t);	
				res = recv_message(&r);
				continue;
			}
		}

		if (res > 0)
		{
			send_message(&t);
			if (strcmp(mod,"parity") == 0)
				point = r.payload + 1;
			else
				point = r.payload;

			token = strtok (strdup(point), " ");
			aux = strtok(NULL," ");

			if  (token != NULL)
			{
				if (strcmp(token,"ls")  == 0)
					res = listareDir(&r,mod,aux);
				else
					if (strcmp(token,"cd")  == 0)
						res = changeDir(&r,aux);
					else
		   				if (strcmp(token,"cp")  == 0)
							res = copyToClient(&r,mod,aux);
						else
							if (strcmp(token,"sn")  == 0)
								res = sentToServer(&r,mod,aux);
							else
								if (strcmp(token,"exit")  == 0)
									exit(0);
			
			}

		}
  
	}
	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}
