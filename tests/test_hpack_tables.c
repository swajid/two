#include "unit.h"
#include "fff.h"
#include "hpack_tables.h"
#include "headers.h"

extern int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value, char *tmp_name, char *tmp_value);
extern const hpack_static_table_t hpack_static_table;
extern int8_t hpack_tables_static_find_entry_name_and_value(uint8_t index, char *name, char *value);
extern int8_t hpack_tables_static_find_entry_name(uint8_t index, char *name);
extern int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
extern int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size);
extern int8_t hpack_tables_dynamic_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
extern int8_t hpack_tables_dynamic_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);
extern int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_size, uint32_t new_max_size);

DEFINE_FFF_GLOBALS;

void setUp(void)
{
    /* Register resets */
    /*FFF_FAKES_LIST(RESET_FAKE);*/

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_hpack_tables_find_index(void)
{
    int expected_index[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16 };
    char *names[] = { ":method", ":method", ":path", ":path", ":scheme", ":scheme", ":status", ":status", ":status", ":status", ":status", ":status", ":status", "accept-encoding" };
    char *values[] = { "GET", "POST", "/", "/index.html", "http", "https", "200", "204", "206", "304", "400", "404", "500", "gzip, deflate" };
    char tmp_name[MAX_HEADER_NAME_LEN];
    char tmp_value[MAX_HEADER_VALUE_LEN];

    for (int i = 0; i < 14; i++) {
        int actual_index = hpack_tables_find_index(NULL, names[i], values[i], tmp_name, tmp_value);
        TEST_ASSERT_EQUAL(expected_index[i], actual_index);
    }

#if HPACK_INCLUDE_DYNAMIC_TABLE
    //Now little test with dynamic table_length
    uint16_t dynamic_table_max_size = 500;
    hpack_dynamic_table_t dynamic_table;

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "hola", "sol3", "bien1" };
    char *new_values[] = { "chao", "luna4", "mal2" };
    int expected_index_dynamic[] = { 64, 63, 62 };
    //added to dynamic table
    for (int i = 0; i < 3; i++) {
        hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[i], new_values[i]);
    }

    for (int i = 0; i < 3; i++) {
        int actual_index = hpack_tables_find_index(&dynamic_table, new_names[i], new_values[i], tmp_name, tmp_value);
        TEST_ASSERT_EQUAL(expected_index_dynamic[i], actual_index);
    }
#endif
}

void test_hpack_tables_static_find_entry_name_and_value(void)
{
    char *expected_name[] = { ":authority", ":method", ":method", "accept-encoding", ":status", "content-range", "if-unmodified-since" };
    char *expected_value[] = { "", "POST", "GET", "gzip, deflate", "404", "", "" };
    uint32_t example_index[] = { 1, 3, 2, 16, 13, 30, 43 };

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];

    for (int i = 0; i < 7; i++) {
        memset(name, 0, sizeof(name));
        memset(value, 0, sizeof(value));
        hpack_tables_static_find_entry_name_and_value(example_index[i], name, value);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_value[i], value);
    }
}

void test_hpack_tables_static_find_entry_name(void)
{
    char *expected_name[] = { ":path", "age", "accept", "allow", "cookie" };
    uint32_t example_index[] = { 5, 21, 19, 22, 32 };

    char name[MAX_HEADER_NAME_LEN];

    for (int i = 0; i < 5; i++) {
        memset(name, 0, sizeof(name));
        hpack_tables_static_find_entry_name(example_index[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
    }
}

#if HPACK_INCLUDE_DYNAMIC_TABLE
void test_hpack_tables_dynamic_add_find_entry_and_reset_table(void)
{
    uint16_t dynamic_table_max_size = 500;
    hpack_dynamic_table_t dynamic_table;

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "hola", "sol3", "bien1" };
    char *new_values[] = { "chao", "luna4", "mal2" };

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];

    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(0, dynamic_table.next);

    //test error case, in the beginning there isn't any entry
    int8_t rc_fail = hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); // 62 -> doesn't exist ...
    TEST_ASSERT_EQUAL(-1, rc_fail);


    //ITERATION 1, add first entry
    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[0], new_values[0]);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(10, dynamic_table.next);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[0], name);
    TEST_ASSERT_EQUAL_STRING(new_values[0], value);

    //ITERATION 2, add second entry
    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[1], new_values[1]);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(21, dynamic_table.next);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value); //63 -> second value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[0], name);
    TEST_ASSERT_EQUAL_STRING(new_values[0], value);

    //ITERATION 3, add third entry
    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[2], new_values[2]);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(32, dynamic_table.next);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[2], name);
    TEST_ASSERT_EQUAL_STRING(new_values[2], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value); //63 -> second value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 64, name, value); //64 -> third value of dynamic table
    TEST_ASSERT_EQUAL_STRING(new_names[0], name);
    TEST_ASSERT_EQUAL_STRING(new_values[0], value);

    //now errors case tests, when entry doesn't exist
    rc_fail = hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 65, name, value); // 65 -> doesn't exist ...
    TEST_ASSERT_EQUAL(-1, rc_fail);

    //Now try reset the table, resizing to 0 and then back to normal
    hpack_tables_dynamic_table_resize(&dynamic_table, HPACK_MAX_DYNAMIC_TABLE_SIZE,0 );
    TEST_ASSERT_EQUAL(0, dynamic_table.n_entries);
    TEST_ASSERT_EQUAL(0, dynamic_table.max_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.actual_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(0, dynamic_table.next);
    hpack_tables_dynamic_table_resize(&dynamic_table, HPACK_MAX_DYNAMIC_TABLE_SIZE, dynamic_table_max_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.n_entries);
    TEST_ASSERT_EQUAL(dynamic_table_max_size, dynamic_table.max_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.actual_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(0, dynamic_table.next);

}

void test_hpack_tables_dynamic_pop_old_entry(void)
{
    uint16_t dynamic_table_max_size = 64 + 20; //aprox size of two standard entries -> 2*(32 + 10chars)
    hpack_dynamic_table_t dynamic_table;

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "hola1", "sol2", "bota3" };
    char *new_values[] = { "chao1", "luna2", "pato3" };

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];

    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(0, dynamic_table.next);

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[0], new_values[0]);
    TEST_ASSERT_EQUAL(1, dynamic_table.n_entries);

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[1], new_values[1]);
    TEST_ASSERT_EQUAL(2, dynamic_table.n_entries);

    //now it has to delete the old entry
    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[2], new_values[2]);
    TEST_ASSERT_EQUAL(2, dynamic_table.n_entries);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[2], name);
    TEST_ASSERT_EQUAL_STRING(new_values[2], value);


}

void test_hpack_tables_dynamic_circular_test(void)
{
    uint16_t dynamic_table_max_size = 32 + 100; //aprox size of one standard entry -> 1*(32 + 100chars)
    hpack_dynamic_table_t dynamic_table;

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "Dias antes de la devastacion paso algo terrible, los piratas se llevaron a Ellie, quien lo diria",
                          "No se que ocurrio, pero todo indicaba que las cosas no iban a mejorar" };
    char *new_values[] = { "!",
                           "..." };

    char name[100]; //example buffers, in reality they are not so big
    char value[100];

    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(0, dynamic_table.next);

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[0], new_values[0]);
    TEST_ASSERT_EQUAL(1, dynamic_table.n_entries);

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[1], new_values[1]);
    TEST_ASSERT_EQUAL(1, dynamic_table.n_entries);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);


}

void test_hpack_tables_dynamic_resize_not_circular(void)
{
    //this is the simple case of resize, first pointer before next pointer
    uint16_t dynamic_table_max_size = 500;
    hpack_dynamic_table_t dynamic_table;

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "hola", "sol3", "bien1" };
    char *new_values[] = { "chao", "luna4", "mal2" };
    //SIZE of entries:
    /*
     * hola, chao = 8 + 32
     * sol3, luna4 = 9 + 32
     * bien1, mal2 = 9 + 32
     * TOTAL SIZE = 122
     */

    for (int i = 0; i < 3; i++) {
        hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[i], new_values[i]);
    }

    // so if a resize is made to size 100, the old entry must be deleted and everything should work normal
    hpack_tables_dynamic_table_resize(&dynamic_table,  HPACK_MAX_DYNAMIC_TABLE_SIZE ,100);
    TEST_ASSERT_EQUAL(2, dynamic_table.n_entries);
    TEST_ASSERT_EQUAL(100, dynamic_table.max_size);
    TEST_ASSERT_EQUAL(82, dynamic_table.actual_size);
    TEST_ASSERT_EQUAL(0, dynamic_table.first);
    TEST_ASSERT_EQUAL(22, dynamic_table.next);

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[2], name);
    TEST_ASSERT_EQUAL_STRING(new_values[2], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    int8_t rc = hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 64, name, value);
    TEST_ASSERT_EQUAL(-1, rc);  // cause entry doesnt exist

}

void test_hpack_tables_dynamic_resize_circular(void)
{
    uint16_t dynamic_table_max_size = 32 + 100; //aprox size of one standard entry -> 1*(32 + 100chars)
    hpack_dynamic_table_t dynamic_table;

    //Current implementation of resize has optimize the sorting (only if first > next) in two cases,
    // 1. If |[next]| <= |[first, max_size]|
    // 2. otherwise ...

    //SUB TEST 1: Optimization 2

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_names[] = { "Dias antes de la devastacion paso algo terrible, los piratas se llevaron a Ellie, quien lo diria",
                          "No se que ocurrio, pero todo indicaba que las cosas no iban a mejorar" };
    char *new_values[] = { "!",
                           "..." };

    char name[100]; //example buffers, in reality they are not so big
    char value[100];
    for (int i = 0; i < 2; i++) {
        hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[i], new_values[i]);
    }
    //now  tables is working circular, pointer to first is after pointer to next
    //something like this -> |omo estas?xxxxxxxxxxxxxxxxhola c|
    TEST_ASSERT_EQUAL(1, dynamic_table.next <= dynamic_table.first);
    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);
    TEST_ASSERT_EQUAL(1, dynamic_table.next > (dynamic_table.max_size - dynamic_table.first)); // this proves that is case *2*
    //resize to the same size, only to prove "sort" algorithm
    //now it has to be instead ->|hola como estas?xxxxxxxxx...|
    hpack_tables_dynamic_table_resize(&dynamic_table,  HPACK_MAX_DYNAMIC_TABLE_SIZE, dynamic_table_max_size);

    TEST_ASSERT_EQUAL(1, dynamic_table.first < dynamic_table.next);
    TEST_ASSERT_EQUAL(1, dynamic_table.n_entries);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values[1], value);

    //SUBTEST 2: Optimization 1
    // Previous part of test tested optimization 2, so now 1 is left to test...

    //first reset dynamic table
    hpack_tables_dynamic_table_resize(&dynamic_table, HPACK_MAX_DYNAMIC_TABLE_SIZE, 0);
    hpack_tables_dynamic_table_resize(&dynamic_table, HPACK_MAX_DYNAMIC_TABLE_SIZE, dynamic_table_max_size);
    char *new_names1[] = { "Dias antes de la devastacion paso algo terrible, los piratas se llevaron a Ellie, quien lo diria",
                           "Creo que todo fue un sueño, que raro!" };
    char *new_values1[] = { "!",
                            "..." };
    for (int i = 0; i < 2; i++) {
        hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names1[i], new_values1[i]);
    }

    TEST_ASSERT_EQUAL(1, dynamic_table.next <= dynamic_table.first);
    TEST_ASSERT_EQUAL(1, dynamic_table.next <= (dynamic_table.max_size - dynamic_table.first)); // this proves that is case *1*
    hpack_tables_dynamic_table_resize(&dynamic_table, HPACK_MAX_DYNAMIC_TABLE_SIZE, dynamic_table_max_size);

    TEST_ASSERT_EQUAL(1, dynamic_table.first < dynamic_table.next);
    TEST_ASSERT_EQUAL(1, dynamic_table.n_entries);

    memset(name, 0, sizeof(name));
    memset(value, 0, sizeof(value));
    hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value);
    TEST_ASSERT_EQUAL_STRING(new_names1[1], name);
    TEST_ASSERT_EQUAL_STRING(new_values1[1], value);

}
#endif

void test_hpack_tables_find_entry(void)
{
#if HPACK_INCLUDE_DYNAMIC_TABLE

    uint16_t dynamic_table_max_size = 500;
    hpack_dynamic_table_t dynamic_table;

    uint32_t example_index[] = { 1, 2, 3, 62, 61 };
    char *expected_name[] = { ":authority", ":method", ":method", "hola", "www-authenticate" };
    char *expected_value[] = { "", "GET", "POST", "chao", "" };

    hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size);

    char *new_name = "hola";
    char *new_value = "chao";

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_name, new_value);

    for (int i = 0; i < 5; i++) {
        memset(name, 0, sizeof(name));
        memset(value, 0, sizeof(value));
        hpack_tables_find_entry_name_and_value(&dynamic_table, example_index[i], name, value);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_value[i], value);
    }
    for (int i = 0; i < 5; i++) {
        memset(name, 0, sizeof(name));
        hpack_tables_find_entry_name(&dynamic_table, example_index[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
    }
#else
    uint32_t example_index[] = { 1, 2, 3, 61 };
    char *expected_name[] = { ":authority", ":method", ":method", "www-authenticate" };
    char *expected_value[] = { "", "GET", "POST", "" };

    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
    for (int i = 0; i < 4; i++) {
        memset(name, 0, sizeof(name));
        memset(value, 0, sizeof(value));
        hpack_tables_find_entry_name_and_value(NULL, example_index[i], name, value);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_value[i], value);
    }
    for (int i = 0; i < 4; i++) {
        memset(name, 0, sizeof(name));
        hpack_tables_find_entry_name(NULL, example_index[i], name);
        TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
    }
#endif
}


int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_hpack_tables_find_index);
    UNIT_TEST(test_hpack_tables_static_find_entry_name_and_value);
    UNIT_TEST(test_hpack_tables_static_find_entry_name);
    UNIT_TEST(test_hpack_tables_find_entry);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    UNIT_TEST(test_hpack_tables_dynamic_add_find_entry_and_reset_table);
    UNIT_TEST(test_hpack_tables_dynamic_pop_old_entry);
    UNIT_TEST(test_hpack_tables_dynamic_circular_test);
    UNIT_TEST(test_hpack_tables_dynamic_resize_not_circular);
    UNIT_TEST(test_hpack_tables_dynamic_resize_circular);
#endif
    return UNITY_END();
}
