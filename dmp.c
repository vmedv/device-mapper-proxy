// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2003 Jana Saout <jana@saout.de>
 *
 * This file is released under the GPL.
 */

#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

#define DM_MSG_PREFIX "dmp"

typedef struct dmp_stat {
  size_t reqs;
  size_t blk_size_sum;
} dmp_stat;

typedef struct dmp_dev {
  struct dm_dev* dev;
  dmp_stat read;
  dmp_stat write;
} dmp_dev;


// overall stat
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
  struct dmp_dev* dt = kmalloc(sizeof(dmp_dev), GFP_KERNEL);

  if (dt) {
    ti->error = "dm-basic_target: Cannot allocate linear context";
    return -ENOMEM;
  }

  if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dt->dev)) {
    ti->error = "dm-basic_target: Device lookup failed";
    return -EINVAL;
  }

  dt->read.blk_size_sum = 0;
  dt->read.reqs = 0;
  dt->write.blk_size_sum = 0;
  dt->write.reqs = 0;
  ti->private = dt;

	return 0;
}

/*
 * Return zeros only on reads
 */
static int dmp_map(struct dm_target *ti, struct bio *bio)
{
  struct dmp_dev* dt = (dmp_dev*) ti->private;
  bio->bi_bdev = dt->dev->bdev;
  switch (bio_op(bio)) {
    case REQ_OP_READ:
      size_t sz = bio_sectors(bio) * SECTOR_SIZE;
      dt->read.reqs++;
      dt->read.blk_size_sum += sz;
      dmp_read.reqs++;
      dmp_read.blk_size_sum += sz;
      break;
    case REQ_OP_WRITE:
      size_t sz = bio_sectors(bio) * SECTOR_SIZE;
      dt->write.reqs++;
      dt->write.blk_size_sum += sz;
      dmp_write.reqs++;
      dmp_write.blk_size_sum += sz;
      break;
    default:
      break;
	}

	bio_endio(bio);

	/* accepted bio, don't make new request */
	return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target *ti)
{
  struct dmp_dev* dt = (dmp_dev*) ti->private;
  dm_put_device(ti, dt->dev);
  kfree(dt);
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

MODULE_DESCRIPTION(DM_NAME " log simple stats");
MODULE_LICENSE("GPL");
