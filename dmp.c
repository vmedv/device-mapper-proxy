// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2003 Jana Saout <jana@saout.de>
 *
 * This file is released under the GPL.
 */

#include <asm-generic/errno-base.h>
#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

#define DM_MSG_PREFIX "dmp"

typedef struct dmp_stat {
  size_t reqs;
  size_t blk_size_sum;
} dmp_stat;

dmp_stat dmp_read, dmp_write;

/*
 * Construct a dummy mapping that only returns zeros
 */
static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	if (argc != 2) {
		ti->error = "Invalid argument count";
		return -EINVAL;
	}
  struct dm_dev* dt;
  if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dt)) {
    ti->error = "dm-basic_target: Device lookup failed";
    return -EINVAL;
  }

  ti->private = dt;

	return 0;
}

/*
 * Return zeros only on reads
 */
static int dmp_map(struct dm_target *ti, struct bio *bio)
{
  struct dm_dev* dt = (struct dm_dev*) ti->private;
  bio->bi_bdev = dt->bdev;
  switch (bio_op(bio)) {
    case REQ_OP_READ:
      dmp_read.reqs++;
      dmp_read.blk_size_sum += bio_sectors(bio) * SECTOR_SIZE;
      break;
    case REQ_OP_WRITE:
      dmp_write.reqs++;
      dmp_write.blk_size_sum += bio_sectors(bio) * SECTOR_SIZE;
      break;
    case REQ_OP_DISCARD:
      /* writes get silently dropped */
      break;
    default:
		return DM_MAPIO_KILL;
	}

	bio_endio(bio);

	/* accepted bio, don't make new request */
	return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target *ti)
{
  dm_put_device(ti, ti->private);
}

static struct target_type dmp_target = {
	.name   = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr    = dmp_ctr,
	.map    = dmp_map,
  .dtr    = dmp_dtr,
};
module_dm(dmp);

MODULE_DESCRIPTION(DM_NAME " ");
MODULE_LICENSE("GPL");
