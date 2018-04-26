/*
Much thanks to Evgeniy Polyakov
This file itself is a snapshop version of netlink.h by Evgeniy Polyakov
*/

/*
 * w1_netlink.h
 *
 * Copyright (c) 2003 Evgeniy Polyakov <johnpol@2ka.mipt.ru>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __W1_NETLINK_H
#define __W1_NETLINK_H

//#include <sys/types.h>

#ifndef CN_W1_IDX
#define CN_W1_IDX      4
#define CN_W1_VAL      1
#endif

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

enum w1_netlink_message_types {
	W1_SLAVE_ADD = 0,
	W1_SLAVE_REMOVE,
	W1_MASTER_ADD,
	W1_MASTER_REMOVE,
	W1_MASTER_CMD,
	W1_SLAVE_CMD,
	W1_LIST_MASTERS,
};

struct w1_netlink_msg
{
	__u8				type;
	__u8				status;
	__u16				len;
	union {
		__u8			id[8];
		struct w1_mst {
			__u32		id;
			__u32		res;
		} mst;
	} id;
	__u8				data[0];
};

enum w1_commands {
	W1_CMD_READ = 0 ,
	W1_CMD_WRITE ,
	W1_CMD_SEARCH,
	W1_CMD_ALARM_SEARCH,
	W1_CMD_TOUCH,
	W1_CMD_RESET,
	W1_CMD_MAX,
} ;

struct w1_netlink_cmd
{
	__u8				cmd;
	__u8				res;
	__u16				len;
	__u8				data[0];
};
#pragma pack(pop)   /* restore original alignment from stack */

#endif /* __W1_NETLINK_H */
