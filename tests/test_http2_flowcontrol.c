#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2_flowcontrol.h"
#include "logging.h"


void setUp(void)
{
  //FFF_RESET_HISTORY();
}

void test_get_window_available_size(void){
  h2_window_manager_t window;
  window.window_size = 20;
  window.window_used = 10;
  uint32_t size = get_window_available_size(window);
  TEST_ASSERT_MESSAGE(size == 10, "Available size must be 10");
}

void test_increase_window_used(void){
  h2_window_manager_t window;
  window.window_used = 0;
  int rc = increase_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 0, "Size of window used must be 0");
  rc = increase_window_used(&window, 10);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 10, "Size of window used must be 10");
  rc = increase_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 10, "Size of window used must be 10");
  rc = increase_window_used(&window, 300);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 310, "Size of window used must be 310");
}

void test_decrease_window_used(void){
  h2_window_manager_t window;
  window.window_used = 0;
  int rc = decrease_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 0, "Size of window used must be 0");
  window.window_used = 30;
  rc = decrease_window_used(&window, 10);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 20, "Size of window used must be 20");
  rc = decrease_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 20, "Size of window used must be 20");
}

void test_flow_control_receive_data(void){
  hstates_t st;
  st.h2s.incoming_window.window_size = 2888;
  st.h2s.incoming_window.window_used = 0;
  int rc = flow_control_receive_data(&st, 10);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.incoming_window.window_used == 10, "Size of window used must be 10");
  rc = flow_control_receive_data(&st, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.incoming_window.window_used == 10, "Size of window used must be 10");
}

void test_flow_control_receive_data_error(void){
  hstates_t st;
  st.h2s.incoming_window.window_size = 2888;
  st.h2s.incoming_window.window_used = 0;
  int rc = flow_control_receive_data(&st, 2900);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (length > window_available)");
  TEST_ASSERT_MESSAGE(st.h2s.incoming_window.window_used == 0, "Size of window used must be 0");
}

void test_flow_control_send_window_update(void){
  hstates_t st;
  st.h2s.incoming_window.window_size = 2888;
  st.h2s.incoming_window.window_used = 30;
  int rc = flow_control_send_window_update(&st, 10);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.incoming_window.window_used == 20, "Size of window used must be 20");
  rc = flow_control_send_window_update(&st, 20);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.incoming_window.window_used == 0, "Size of window used must be 0");
}

void test_flow_control_send_window_update_error(void){
  hstates_t st;
  st.h2s.incoming_window.window_size = 2888;
  st.h2s.incoming_window.window_used = 0;
  int rc = flow_control_send_window_update(&st, 20);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1, (window_size_increment > window_used)");
}

void test_flow_control_receive_window_update(void){
  hstates_t st;
  st.h2s.outgoing_window.window_size = 2888;
  st.h2s.outgoing_window.window_used = 40;
  int rc = flow_control_receive_window_update(&st, 20);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.outgoing_window.window_used == 20, "Size of window used must be 20");
}

void test_flow_control_receive_window_update_error(void){
  hstates_t st;
  st.h2s.outgoing_window.window_size = 2888;
  st.h2s.outgoing_window.window_used = 0;
  int rc = flow_control_receive_window_update(&st, 20);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
  TEST_ASSERT_MESSAGE(st.h2s.outgoing_window.window_used == 0, "Size of window used must be 0");

}

void test_get_size_data_to_send(void){
    hstates_t st;
    st.headers_in.count = 0;
    st.headers_out.count = 0;
    st.data_in.size = 0;
    st.data_out.size = 0;
    st.data_in.processed = 0;
    st.data_out.processed = 0;
    st.h2s.outgoing_window.window_size = 10;
    st.h2s.outgoing_window.window_used = 0;
    st.data_out.size = 10;
    st.data_out.processed = 0;
    uint32_t rc = get_size_data_to_send(&st);
    TEST_ASSERT_EQUAL(10,rc);

    st.data_out.size = 5;
    st.data_out.processed = 0;
    rc = get_size_data_to_send(&st);
    TEST_ASSERT_EQUAL(5,rc);

    st.data_out.size = 15;
    st.data_out.processed = 0;
    rc = get_size_data_to_send(&st);
    TEST_ASSERT_EQUAL(10,rc);
}


int main(void)
{
  UNIT_TESTS_BEGIN();
  UNIT_TEST(test_get_window_available_size);
  UNIT_TEST(test_increase_window_used);
  UNIT_TEST(test_decrease_window_used);
  UNIT_TEST(test_flow_control_receive_data);
  UNIT_TEST(test_flow_control_receive_data_error);
  UNIT_TEST(test_flow_control_send_window_update);
  UNIT_TEST(test_flow_control_send_window_update_error);
  UNIT_TEST(test_flow_control_receive_window_update);
  UNIT_TEST(test_flow_control_receive_window_update_error);
  UNIT_TEST(test_get_size_data_to_send);
  return UNIT_TESTS_END();
}
