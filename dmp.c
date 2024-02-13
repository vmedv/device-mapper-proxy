#include <linux/device-mapper.h>
#include <linux/kobject.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

#define DM_MSG_PREFIX "dmp"

typedef struct dmp_stat {
	size_t reqs;
	size_t blk_size_sum;
} dmp_stat;

typedef struct dmp_dev {
  struct kobject kobj;
	struct dm_dev *dev;
	dmp_stat read;
	dmp_stat write;
} dmp_dev;

#define kobj_to_dmp_dev(kobj) container_of(kobj, struct dmp_dev, kobj)

static ssize_t dm_attr_stat_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
  dmp_dev* dd = kobj_to_dmp_dev(kobj);
  return sprintf(buf, "read:\n\treqs: %ld\n\tavg size: %ld\nwrite:\n\treqs: %ld\n\tavg size: %ld\ntotal:\n\treqs: %ld\n\tavg size: %ld\n", 
                 dd->read.reqs, 
                 dd->read.blk_size_sum ? dd->read.blk_size_sum / dd->read.reqs : 0,
                 dd->write.reqs,
                 dd->write.blk_size_sum ? dd->write.blk_size_sum / dd->write.reqs : 0,
                 dd->read.reqs + dd->write.reqs,
                 dd->read.blk_size_sum + dd->write.blk_size_sum ? (dd->read.blk_size_sum + dd->write.blk_size_sum) / (dd->read.reqs + dd->write.reqs) : 0
                 );
}

void dmp_dev_release(struct kobject *kobj)
{
  dmp_dev* dd = kobj_to_dmp_dev(kobj);
  kfree(dd);
}

static struct attribute attr = {
  .name = "stat",
  .mode = 044,
};

static struct sysfs_ops dmp_dev_ops = {
  .show = dm_attr_stat_show,
};

static struct kobj_type dmp_dev_ktype = {
  .release = dmp_dev_release,
  .sysfs_ops = &dmp_dev_ops,
};

// static struct kobj_attribute stat_attr = __ATTR(stat, 0444, dm_attr_stat_show, NULL);

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
  if (argc != 1) {
    ti->error = "dmp: Invalid argument count";
    return -EINVAL;
  }

	struct dmp_dev *dt = kmalloc(sizeof(dmp_dev), GFP_KERNEL);

	if (!dt) {
		ti->error = "dmp: Cannot allocate linear context";
		return -ENOMEM;
	}
	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table),
			  &dt->dev)) {
		ti->error = "dmp: Device lookup failed";
		return -EINVAL;
	}

	dt->read.blk_size_sum = 0;
	dt->read.reqs = 0;
	dt->write.blk_size_sum = 0;
	dt->write.reqs = 0;
  kobject_init_and_add(&dt->kobj, &dmp_dev_ktype, &(THIS_MODULE->mkobj.kobj), strrchr(argv[0], '/') + 1);
  int error = sysfs_create_file(&dt->kobj, &attr);
  if (error) {
    dm_put_device(ti, dt->dev);
    kobject_put(&dt->kobj);
    return error;
  }
	ti->private = dt;

  return 0;
}

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	struct dmp_dev *dt = (dmp_dev *)ti->private;
	bio->bi_bdev = dt->dev->bdev;
	size_t sz;
	switch (bio_op(bio)) {
	case REQ_OP_READ:
		sz = bio_sectors(bio) * SECTOR_SIZE;
		dt->read.reqs++;
		dt->read.blk_size_sum += sz;
		break;
	case REQ_OP_WRITE:
		sz = bio_sectors(bio) * SECTOR_SIZE;
		dt->write.reqs++;
		dt->write.blk_size_sum += sz;
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
	struct dmp_dev *dt = (dmp_dev *)ti->private;
	dm_put_device(ti, dt->dev);
  kobject_put(&dt->kobj);
}

static struct target_type dmp_target = {
	.name = "dmp",
	.version = { 1, 0, 0 },
	.module = THIS_MODULE,
	.ctr = dmp_ctr,
	.map = dmp_map,
	.dtr = dmp_dtr,
};
module_dm(dmp);

MODULE_DESCRIPTION(DM_NAME " log simple stats");
MODULE_LICENSE("GPL");
