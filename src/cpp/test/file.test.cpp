// Copyright 2017 The Procyon Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <procyon.h>

#include <gmock/gmock.h>
#include <limits>

#include "./matchers.hpp"

using FileTest = ::testing::Test;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::NotNull;

namespace pntest {

TEST_F(FileTest, ReadMode) {
    pn_value_t file;
    pn_set(&file, 's', "10 20 30");
    pn_file_t* f = pn_open_string(&file.s, "r");
    ASSERT_THAT(f, NotNull()) << strerror(errno);

    int i;
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(10));
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(20));
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(30));

    ASSERT_THAT(fseek(f, 0, SEEK_SET), Eq(0));
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(10));

    ASSERT_THAT(fseek(f, 3, SEEK_CUR), Eq(0));
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(30));

    ASSERT_THAT(fseek(f, -6, SEEK_END), Eq(0));
    ASSERT_THAT(fscanf(f, "%d", &i), Gt(0));
    ASSERT_THAT(i, Eq(20));

    errno = 0;
    ASSERT_THAT(fseek(f, -1, SEEK_SET), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));
    errno = 0;
    ASSERT_THAT(fseek(f, -9, SEEK_CUR), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));
    errno = 0;
    ASSERT_THAT(fseek(f, -9, SEEK_END), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));

    ASSERT_THAT(ferror(f), Eq(false));
    ASSERT_THAT(fprintf(f, "%s", "haha!"), Lt(0));
    ASSERT_THAT(ferror(f), Eq(true));

    fclose(f);
    pn_clear(&file);
}

TEST_F(FileTest, WriteMode) {
    pn_value_t file;
    pn_set(&file, 's', "truncate");
    pn_file_t* f = pn_open_string(&file.s, "w");
    ASSERT_THAT(f, NotNull()) << strerror(errno);
    setbuf(f, NULL);

    ASSERT_THAT(fprintf(f, "testing %d %d %d", 1, 2, 3), Gt(0));
    ASSERT_THAT(file, IsString("testing 1 2 3"));

    ASSERT_THAT(fseek(f, 0, SEEK_SET), Eq(0));
    ASSERT_THAT(fprintf(f, "pass"), Gt(0));
    ASSERT_THAT(file, IsString("passing 1 2 3"));

    ASSERT_THAT(fseek(f, 8, SEEK_CUR), Eq(0));
    ASSERT_THAT(fprintf(f, "4 5"), Gt(0));
    ASSERT_THAT(file, IsString("passing 1 2 4 5"));

    ASSERT_THAT(fseek(f, 1, SEEK_END), Eq(0));
    ASSERT_THAT(fprintf(f, "6"), Gt(0));
    ASSERT_THAT(file, IsString(std::string("passing 1 2 4 5\0006", 17)));

    errno = 0;
    ASSERT_THAT(fseek(f, -1, SEEK_SET), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));
    errno = 0;
    ASSERT_THAT(fseek(f, -18, SEEK_CUR), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));
    errno = 0;
    ASSERT_THAT(fseek(f, -18, SEEK_END), Lt(0));
    ASSERT_THAT(errno, Eq(EINVAL));

    uint8_t u8;
    ASSERT_THAT(ferror(f), Eq(false));
    ASSERT_THAT(fread(&u8, sizeof(u8), 1, f), Eq(0u));
    ASSERT_THAT(ferror(f), Eq(true));

    fclose(f);
    pn_clear(&file);
}

TEST_F(FileTest, AppendMode) {
    pn_value_t file;
    pn_set(&file, 's', "alpha\n");
    pn_file_t* f = pn_open_string(&file.s, "a");
    ASSERT_THAT(f, NotNull()) << strerror(errno);
    setbuf(f, NULL);

    ASSERT_THAT(fprintf(f, "beta\n"), Gt(0));
    ASSERT_THAT(fprintf(f, "gamma\n"), Gt(0));
    ASSERT_THAT(file, IsString("alpha\nbeta\ngamma\n"));

    uint8_t u8;
    ASSERT_THAT(ferror(f), Eq(false));
    ASSERT_THAT(fread(&u8, sizeof(u8), 1, f), Eq(0u));
    ASSERT_THAT(ferror(f), Eq(true));

    fclose(f);
    pn_clear(&file);
}

TEST_F(FileTest, WriteBuffered) {
    pn_value_t file;
    pn_set(&file, 's', "truncate");
    pn_file_t* f = pn_open_string(&file.s, "w");
    ASSERT_THAT(f, NotNull()) << strerror(errno);

    ASSERT_THAT(fprintf(f, "first"), Gt(0));
    ASSERT_THAT(fflush(f), Eq(0));
    ASSERT_THAT(file, IsString("first"));

    ASSERT_THAT(fprintf(f, " second"), Gt(0));
    fclose(f);
    ASSERT_THAT(file, IsString("first second"));

    pn_clear(&file);
}

TEST_F(FileTest, PlusMode) {
    pn_value_t file;
    pn_set(&file, 's', "truncate");

    {
        pn_file_t* f = pn_open_string(&file.s, "w+");
        ASSERT_THAT(f, NotNull()) << strerror(errno);
        setbuf(f, NULL);

        ASSERT_THAT(fprintf(f, "w"), Gt(0));
        ASSERT_THAT(file, IsString("w"));

        ASSERT_THAT(fseek(f, 0, SEEK_SET), Eq(0));
        ASSERT_THAT(fgetc(f), Eq('w'));

        ASSERT_THAT(ferror(f), Eq(false));
        fclose(f);
    }

    {
        pn_file_t* f = pn_open_string(&file.s, "a+");
        ASSERT_THAT(f, NotNull()) << strerror(errno);
        setbuf(f, NULL);

        ASSERT_THAT(fprintf(f, "a"), Gt(0));
        ASSERT_THAT(file, IsString("wa"));

        ASSERT_THAT(fseek(f, -1, SEEK_CUR), Eq(0));
        ASSERT_THAT(fgetc(f), Eq('a'));

        ASSERT_THAT(ferror(f), Eq(false));
        fclose(f);
    }

    {
        pn_file_t* f = pn_open_string(&file.s, "r+");
        ASSERT_THAT(f, NotNull()) << strerror(errno);
        setbuf(f, NULL);

        ASSERT_THAT(fgetc(f), Eq('w'));

        ASSERT_THAT(fseek(f, 0, SEEK_END), Eq(0));
        ASSERT_THAT(fprintf(f, "r"), Gt(0));
        ASSERT_THAT(file, IsString("war"));

        ASSERT_THAT(ferror(f), Eq(false));
        fclose(f);
    }

    pn_clear(&file);
}

TEST_F(FileTest, BadArgs) {
    pn_value_t file;
    pn_set(&file, 's', "");

    errno = 0;
    ASSERT_THAT(pn_open_string(nullptr, "w"), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    errno = 0;
    ASSERT_THAT(pn_open_string(&file.s, nullptr), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    errno = 0;
    ASSERT_THAT(pn_open_string(&file.s, ""), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    errno = 0;
    ASSERT_THAT(pn_open_string(&file.s, "+"), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    errno = 0;
    ASSERT_THAT(pn_open_string(&file.s, "f"), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    errno = 0;
    ASSERT_THAT(pn_open_string(&file.s, "r++"), Eq(nullptr));
    ASSERT_THAT(errno, Eq(EINVAL));

    pn_clear(&file);
}

}  // namespace pntest
