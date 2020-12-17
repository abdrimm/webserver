#include <stdio.h> 

int main(int argc, char *argv[]) {
	printf("<html> Hello World\n");
	for(int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
	printf("</html>");
	return 0;
}
