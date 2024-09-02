#include "lab5_common.h"
#include "linux/gfp_types.h"
#include "linux/netlink.h"
#include "net/netlink.h"
/*#include <linux/genetlink.h>*/
/*#include <linux/netlink.h>*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <net/genetlink.h>

#define SUCCESS 0
#define MSG "Hello from kernel space"

static struct nla_policy lab5_genl_policy[LAB5_ATTR_MAX + 1] = {
    [LAB5_ATTR_MSG] = {.type = NLA_NUL_STRING},
};

static struct genl_family lab5_genl_family;

static int lab5_echo(struct sk_buff *skb, struct genl_info *info) {
  char *recv_msg;
  size_t msg_size;
  void *msg_head;
  struct sk_buff *skb_out;
  int status;

  pr_info("lab5: handle echo request\n");

  if (!info->attrs[LAB5_ATTR_MSG]) {
    pr_info("lab5: no message received\n");
    return -EINVAL;
  }

  recv_msg = nla_data(info->attrs[LAB5_ATTR_MSG]);
  msg_size = strlen(recv_msg);

  pr_info("lab5: received message: %s\n", recv_msg);

  skb_out = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
  if (!skb_out) {
    pr_info("lab5: failed to alloc skb\n");
    return -ENOMEM;
  }

  msg_head = genlmsg_put(skb_out, info->snd_portid, info->snd_seq + 1,
                         &lab5_genl_family, 0, LAB5_CMD_ECHO);
  if (!msg_head) {
    pr_info("lab5: failed to add genl header\n");
    nlmsg_free(skb_out);
    return -ENOMEM;
  }

  status = nla_put_string(skb_out, LAB5_ATTR_MSG, MSG);
  if (status) {
    pr_info("lab5: failed to add attr\n");
    nlmsg_free(skb_out);
    return status;
  }

  genlmsg_end(skb_out, msg_head);

  status = genlmsg_unicast(genl_info_net(info), skb_out, info->snd_portid);
  if (status) {
    pr_info("lab5: failed to send message\n");
  } else {
    pr_info("lab5: echo message sent, snd_portid=%d\n", info->snd_portid);
  }

  return status;
};

static struct genl_ops lab5_gnl_ops[1] = {{
    .cmd = LAB5_CMD_ECHO,
    .flags = 0,
    .policy = lab5_genl_policy,
    .doit = lab5_echo,
    .dumpit = NULL,
}};

static struct genl_family lab5_genl_family = {
    .id = 0,
    .hdrsize = 0,
    .name = LAB5_FAMILY_NAME,
    .version = 1,
    .maxattr = LAB5_ATTR_MAX,
    .ops = lab5_gnl_ops,
    .n_ops = sizeof(lab5_gnl_ops) / sizeof(struct genl_ops),
};

static int __init lab5_init_module(void) {
  int status = genl_register_family(&lab5_genl_family);
  if (status) {
    pr_info("lab5: failed to register generic netlink family\n");
    return status;
  }

  return SUCCESS;
}

static void __exit lab5_cleanup_module(void) {
  int status = genl_unregister_family(&lab5_genl_family);
  if (status) {
    pr_info("lab5: failed to unregister generic netlink family\n");
    return;
  }
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("danilka108");
MODULE_DESCRIPTION("Danil Churikov eltex summer module 5 lab 5");

module_init(lab5_init_module);
module_exit(lab5_cleanup_module);
