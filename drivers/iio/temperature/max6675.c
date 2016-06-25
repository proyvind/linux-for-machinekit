/*
 * max6675.c - MAX6675 thermocouple converter driver
 *
 * Copyright (C) 2015 Konsulko Group, Matt Porter <mporter@konsulko.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/iio/iio.h>
#include <linux/spi/spi.h>

struct max6675_state {
	struct spi_device *spi;
};

static const struct iio_chan_spec max6675_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_SCALE),
	},
};

static int max6675_read(struct max6675_state *st, int *val)
{
	int ret;

	ret = spi_read(st->spi, val, 2);
	if (ret < 0)
		return ret;

	/* Temperature is bits 14..3 */
	*val = (*val >> 3) & 0xfff;

	return ret;
}

static int max6675_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val,
			    int *val2,
			    long m)
{
	struct max6675_state *st = iio_priv(indio_dev);
	int ret;

	if (m == IIO_CHAN_INFO_RAW) {
		*val2 = 0;
		ret = max6675_read(st, val);
		if (ret)
			return ret;
	} else if (m == IIO_CHAN_INFO_SCALE) {
		*val = 250;
		*val2 = 0;
	} else
		return -EINVAL;

	return IIO_VAL_INT;
}

static const struct iio_info max6675_info = {
	.driver_module = THIS_MODULE,
	.read_raw = &max6675_read_raw,
};

static int max6675_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct max6675_state *st;
	int ret = 0;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);
	st->spi = spi;

	spi->mode = SPI_MODE_1;
	spi->bits_per_word = 16;

	spi_set_drvdata(spi, indio_dev);

	indio_dev->dev.parent = &spi->dev;
	indio_dev->name = spi_get_device_id(spi)->name;
	indio_dev->channels = max6675_channels;
	indio_dev->num_channels = ARRAY_SIZE(max6675_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &max6675_info;

	ret = iio_device_register(indio_dev);
	if (ret < 0)
		dev_err(&spi->dev, "unable to register device\n");

	return ret;
}

static int max6675_remove(struct spi_device *spi)
{
	struct iio_dev *indio_dev = spi_get_drvdata(spi);

	iio_device_unregister(indio_dev);

	return 0;
}

static const struct acpi_device_id max6675_acpi_ids[] = {
	{ "MXIM6675", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, max6675_acpi_ids);

static const struct of_device_id max6675_dt_ids[] = {
	{ .compatible = "maxim,max6675" },
	{},
};
MODULE_DEVICE_TABLE(of, max6675_dt_ids);

static const struct spi_device_id max6675_spi_ids[] = {
	{"max6675", 0},
	{},
};
MODULE_DEVICE_TABLE(spi, max6675_spi_ids);

static struct spi_driver max6675_driver = {
	.driver = {
		.name	= "max6675",
		.owner	= THIS_MODULE,
		.acpi_match_table = ACPI_PTR(max6675_acpi_ids),
		.of_match_table = of_match_ptr(max6675_dt_ids),
	},
	.probe		= max6675_probe,
	.remove		= max6675_remove,
	.id_table	= max6675_spi_ids,
};
module_spi_driver(max6675_driver);

MODULE_AUTHOR("Matt Porter <mporter@konsulko.com>");
MODULE_DESCRIPTION("MAX6675 thermocouple converter driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:max6675");
