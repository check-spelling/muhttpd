#ifndef REQUEST_H
#define REQUEST_H

struct request {
	char *buf;
	char *proto;
	int status;
	char *method;
	char *uri;
	char *filename;
	char *location;
	struct file_type *file_type;
};

void handle_request(struct request *req);
void handle_and_log_request(struct request *req);
void serve();

#endif /* REQUEST_H */
