/******************************************************************************
 *                                                                            *
 * Copyright 2022 MachineWare GmbH                                            *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#include "testing.h"

TEST(disk, ramdisk) {
    log_term log;
    log.set_level(LOG_DEBUG);

    block::disk disk("disk", "ramdisk:8MiB");
    EXPECT_EQ(disk.capacity(), 8 * MiB);
    EXPECT_EQ(disk.pos(), 0);
    EXPECT_EQ(disk.remaining(), disk.capacity());

    u8 a[] = { 0x12, 0x34, 0x56, 0x78 };
    u8 b[] = { 0x00, 0x00, 0x00, 0x00 };

    EXPECT_TRUE(disk.seek(0xffe));
    EXPECT_TRUE(disk.write(a, sizeof(a)));
    EXPECT_TRUE(disk.seek(0xffe));
    EXPECT_TRUE(disk.read(b, sizeof(b)));

    EXPECT_EQ(memcmp(a, b, sizeof(a)), 0);

    EXPECT_EQ(disk.stats.num_bytes_written, sizeof(a));
    EXPECT_EQ(disk.stats.num_bytes_read, sizeof(b));
    EXPECT_EQ(disk.stats.num_write_req, 1);
    EXPECT_EQ(disk.stats.num_read_req, 1);
    EXPECT_EQ(disk.stats.num_seek_req, 2);
    EXPECT_EQ(disk.stats.num_req, 4);
    EXPECT_EQ(disk.stats.num_write_err, 0);
    EXPECT_EQ(disk.stats.num_read_err, 0);
    EXPECT_EQ(disk.stats.num_seek_err, 0);
    EXPECT_EQ(disk.stats.num_err, 0);

    EXPECT_FALSE(disk.seek(8 * MiB + 1));
    EXPECT_TRUE(disk.seek(8 * MiB - 1));
    EXPECT_FALSE(disk.write(a, sizeof(a)));

    EXPECT_EQ(disk.stats.num_bytes_written, sizeof(a));
    EXPECT_EQ(disk.stats.num_bytes_read, sizeof(b));
    EXPECT_EQ(disk.stats.num_write_req, 2);
    EXPECT_EQ(disk.stats.num_read_req, 1);
    EXPECT_EQ(disk.stats.num_seek_req, 4);
    EXPECT_EQ(disk.stats.num_req, 7);
    EXPECT_EQ(disk.stats.num_write_err, 1);
    EXPECT_EQ(disk.stats.num_read_err, 0);
    EXPECT_EQ(disk.stats.num_seek_err, 1);
    EXPECT_EQ(disk.stats.num_err, 2);

    EXPECT_TRUE(disk.seek(4 * MiB));
    EXPECT_TRUE(disk.read(b, sizeof(b)));
    EXPECT_EQ(b[0], 0);
    EXPECT_EQ(b[1], 0);
    EXPECT_EQ(b[2], 0);
    EXPECT_EQ(b[3], 0);

    EXPECT_EQ(disk.stats.num_bytes_written, sizeof(a));
    EXPECT_EQ(disk.stats.num_bytes_read, 8);
    EXPECT_EQ(disk.stats.num_write_req, 2);
    EXPECT_EQ(disk.stats.num_read_req, 2);
    EXPECT_EQ(disk.stats.num_seek_req, 5);
    EXPECT_EQ(disk.stats.num_req, 9);
    EXPECT_EQ(disk.stats.num_write_err, 1);
    EXPECT_EQ(disk.stats.num_read_err, 0);
    EXPECT_EQ(disk.stats.num_seek_err, 1);
    EXPECT_EQ(disk.stats.num_err, 2);
}

static void create_file(const string& path, size_t size) {
    ofstream of(path.c_str(), std::ios::binary | std::ios::out);
    of.seekp(size - 1);
    of.write("\0", 1);
}

TEST(disk, file) {
    log_term log;
    log.set_level(LOG_DEBUG);

    create_file("my.disk", 8 * MiB);

    block::disk disk("disk", "my.disk");
    EXPECT_EQ(disk.capacity(), 8 * MiB);
    EXPECT_EQ(disk.pos(), 0);
    EXPECT_EQ(disk.remaining(), disk.capacity());

    u8 a[] = { 0x12, 0x34, 0x56, 0x78 };
    u8 b[] = { 0x00, 0x00, 0x00, 0x00 };

    EXPECT_TRUE(disk.seek(0xffe));
    EXPECT_TRUE(disk.write(a, sizeof(a)));
    EXPECT_TRUE(disk.seek(0xffe));
    EXPECT_TRUE(disk.read(b, sizeof(b)));

    EXPECT_EQ(memcmp(a, b, sizeof(a)), 0);

    EXPECT_FALSE(disk.seek(8 * MiB + 1));
    EXPECT_TRUE(disk.seek(8 * MiB - 1));
    EXPECT_FALSE(disk.write(a, sizeof(a)));

    std::remove("my.disk");
}

TEST(disk, nothing) {
    log_term log;
    log.set_level(LOG_DEBUG);

    block::disk disk("disk", "nothing");
    EXPECT_EQ(disk.capacity(), 0);
    EXPECT_EQ(disk.pos(), 0);
    EXPECT_EQ(disk.remaining(), disk.capacity());
}

TEST(disk, perm_okay) {
    log_term log;
    log.set_level(LOG_DEBUG);

    create_file("readonly", 1 * MiB);
    chmod("readonly", S_IREAD);

    block::disk disk("disk", "readonly", true);
    EXPECT_EQ(disk.capacity(), 1 * MiB);

    std::remove("readonly");
}

TEST(disk, perm_fail) {
    log_term log;
    log.set_level(LOG_DEBUG);

    create_file("readonly", 1 * MiB);
    chmod("readonly", S_IREAD);

    block::disk disk("disk", "readonly", false);
    EXPECT_EQ(disk.capacity(), 0);

    std::remove("readonly");
}

TEST(disk, serial) {
    create_file("file1", 1 * MiB);
    create_file("file2", 1 * MiB);

    block::disk disk1("disk1", "file1");
    block::disk disk2("disk2", "file2");

    string serial1 = disk1.serial;
    string serial2 = disk2.serial;

    EXPECT_NE(serial1, serial2);

    std::remove("file1");
    std::remove("file2");
}

TEST(ramdisk, unmap_zero) {
    log_term log;
    log.set_level(LOG_DEBUG);

    block::disk disk("disk", "ramdisk:4KiB", false);
    EXPECT_TRUE(disk.wzero(4 * KiB, false));
    EXPECT_TRUE(disk.seek(0));
    EXPECT_TRUE(disk.discard(4 * KiB));

    EXPECT_EQ(disk.stats.num_bytes_written, 4 * KiB);
    EXPECT_EQ(disk.stats.num_seek_req, 1);
    EXPECT_EQ(disk.stats.num_seek_err, 0);
    EXPECT_EQ(disk.stats.num_write_req, 1);
    EXPECT_EQ(disk.stats.num_write_err, 0);
    EXPECT_EQ(disk.stats.num_discard_req, 1);
    EXPECT_EQ(disk.stats.num_discard_err, 0);
    EXPECT_EQ(disk.stats.num_req, 3);
    EXPECT_EQ(disk.stats.num_err, 0);
}
