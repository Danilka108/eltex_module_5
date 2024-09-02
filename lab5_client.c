#include "lab5_common.h"
#include <errno.h>
#include <linux/genetlink.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define GNLHDR_BUF_SIZE 8192

struct gnlhdr_buf {
  char _buff[GNLHDR_BUF_SIZE];
};

#define GNLHDR_BUF(hdr) ((void *)(hdr)->_buff)
#define GNLHDR_BUF_LEN(hdr) (((struct nlmsghdr *)(hdr)->_buff)->nlmsg_len)

void gnlhdr_buf_init(struct gnlhdr_buf *self, u_int16_t type, u_int16_t flags,
                     u_int32_t seq, uint32_t pid, u_int8_t cmd) {
  memset(self->_buff, 0, GNLHDR_BUF_SIZE);

  struct nlmsghdr *nlh = (struct nlmsghdr *)self->_buff;
  struct genlmsghdr *genlh = (struct genlmsghdr *)(self->_buff + NLMSG_HDRLEN);

  nlh->nlmsg_len = NLMSG_HDRLEN + GENL_HDRLEN;
  nlh->nlmsg_type = type;
  nlh->nlmsg_flags = flags;
  nlh->nlmsg_seq = seq;
  nlh->nlmsg_pid = pid;

  genlh->cmd = cmd;
  genlh->version = 2;
}

int gnlhdr_buf_add_attr(struct gnlhdr_buf *self, u_int16_t type,
                        const char *buff, size_t size) {
  struct nlmsghdr *hdr = (struct nlmsghdr *)self->_buff;
  struct nlattr *attr = (struct nlattr *)(self->_buff + hdr->nlmsg_len);

  if (hdr->nlmsg_len + NLA_ALIGN(NLA_HDRLEN + size) > GNLHDR_BUF_SIZE) {
    return -1;
  }

  attr->nla_type = type;
  attr->nla_len = NLA_HDRLEN + size;
  memmove((char *)attr + NLA_HDRLEN, buff, size);

  hdr->nlmsg_len += NLA_ALIGN(NLA_HDRLEN + size);

  return 0;
}

struct gnlhdr_iter {
  struct gnlhdr_buf *buf;
  size_t offset;
};

void gnlhdr_iter_init(struct gnlhdr_iter *iter, struct gnlhdr_buf *buf) {
  iter->buf = buf;
  iter->offset = 0;
}

struct nlattr *gnlhdr_iter_next(struct gnlhdr_iter *iter) {
  struct nlmsghdr *hdr = (struct nlmsghdr *)iter->buf->_buff;
  struct nlattr *attr = (struct nlattr *)(iter->buf->_buff + NLMSG_HDRLEN +
                                          GENL_HDRLEN + iter->offset);

  if ((char *)attr >= iter->buf->_buff + hdr->nlmsg_len) {
    return NULL;
  }

  if ((char *)attr + NLA_ALIGN(attr->nla_len) >
      iter->buf->_buff + hdr->nlmsg_len) {
    return NULL;
  }

  iter->offset += NLA_ALIGN(attr->nla_len);
  return attr;
}

int resolve_family_id(int sfd, const char *family_name) {
  int family_id = -1;
  struct sockaddr_nl saddr;
  struct gnlhdr_buf hdr;
  struct gnlhdr_iter iter;

  memset(&saddr, 0, sizeof(saddr));
  saddr.nl_family = AF_NETLINK;

  gnlhdr_buf_init(&hdr, GENL_ID_CTRL, NLM_F_REQUEST, 1, 0, CTRL_CMD_GETFAMILY);
  gnlhdr_buf_add_attr(&hdr, CTRL_ATTR_FAMILY_NAME, family_name,
                      strlen(family_name) + 1);

  if (sendto(sfd, GNLHDR_BUF(&hdr), GNLHDR_BUF_LEN(&hdr), 0,
             (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("sendto");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  int len = recv(sfd, hdr._buff, GNLHDR_BUF_SIZE, 0);
  if (len < 0) {
    perror("recv");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  gnlhdr_iter_init(&iter, &hdr);
  struct nlattr *attr;

  while ((attr = gnlhdr_iter_next(&iter))) {
    switch (attr->nla_type) {
    case CTRL_ATTR_FAMILY_ID:
      family_id = *(int *)((char *)attr + NLA_HDRLEN);
      printf("family_id = %d\n", family_id);
      break;
    case CTRL_ATTR_FAMILY_NAME:
      printf("family_name = %s\n", (char *)attr + NLA_HDRLEN);
      break;
    }
  }

  return family_id;
}

void send_echo(int sfd, int family_id, const char *message) {
  struct sockaddr_nl saddr;
  struct gnlhdr_buf hdr;
  struct gnlhdr_iter iter;

  memset(&saddr, 0, sizeof(saddr));
  saddr.nl_family = AF_NETLINK;

  gnlhdr_buf_init(&hdr, family_id, NLM_F_REQUEST, 1, 0, LAB5_CMD_ECHO);
  gnlhdr_buf_add_attr(&hdr, LAB5_ATTR_MSG, message, strlen(message) + 1);

  struct nlmsghdr *h = (struct nlmsghdr *)&hdr._buff;

  if (sendto(sfd, GNLHDR_BUF(&hdr), GNLHDR_BUF_LEN(&hdr), 0,
             (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("sendto");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  printf("echo message sent: %s\n", message);

  int len = recv(sfd, GNLHDR_BUF(&hdr), GNLHDR_BUF_SIZE, 0);
  if (len < 0) {
    perror("recv");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  printf("len %d, hdr type %d, seq %d\n", len, h->nlmsg_type, h->nlmsg_flags);

  if (h->nlmsg_type == NLMSG_ERROR) {
    struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
    fprintf(stderr, "error received NLMSG_ERROR, error = %d\n", err->error);
    close(sfd);
    exit(EXIT_FAILURE);
  }

  gnlhdr_iter_init(&iter, &hdr);
  struct nlattr *attr = gnlhdr_iter_next(&iter);

  while (attr) {
    if (attr->nla_type == LAB5_ATTR_MSG) {
      printf("echo response received: %s\n", (char *)attr + NLA_HDRLEN);
    }

    attr = gnlhdr_iter_next(&iter);
  }
}

int main() {
  printf("pid = %d\n", getpid());

  struct sockaddr_nl saddr;

  memset(&saddr, 0, sizeof(saddr));
  saddr.nl_family = AF_NETLINK;
  saddr.nl_pid = getpid();

  int sfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
  if (sfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  int family_id = resolve_family_id(sfd, LAB5_FAMILY_NAME);
  if (family_id < 0) {
    fprintf(stderr, "Failed to resolve family ID for %s\n", LAB5_FAMILY_NAME);
    close(sfd);
    return -1;
  }

  send_echo(sfd, family_id, "Hello from user space");

  close(sfd);
  return 0;
}
