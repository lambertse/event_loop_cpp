#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "../proto/addressbook.pb.h"

class ProtobufQuickTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
  }

  void TearDown() override {
    // Optional: Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
  }
};

TEST_F(ProtobufQuickTest, CreateAndSerializeAddressBook) {
  tutorial::AddressBook address_book;

  // Add a person to the address book
  tutorial::Person* person = address_book.add_people();

  person->set_name("John Doe");
  person->set_id(123);
  person->set_email("john.doe@example.com");

  // Add a phone number
  tutorial::Person::PhoneNumber* phone_number = person->add_phones();
  phone_number->set_number("555-1234");
  phone_number->set_type(tutorial::Person::PHONE_TYPE_HOME);

  // Add another phone number
  phone_number = person->add_phones();
  phone_number->set_number("555-5678");
  phone_number->set_type(tutorial::Person::PHONE_TYPE_MOBILE);

  // Verify the data was set correctly
  EXPECT_EQ(address_book.people_size(), 1);
  EXPECT_EQ(person->name(), "John Doe");
  EXPECT_EQ(person->id(), 123);
  EXPECT_EQ(person->email(), "john.doe@example.com");
  EXPECT_EQ(person->phones_size(), 2);
  EXPECT_EQ(person->phones(0).number(), "555-1234");
  EXPECT_EQ(person->phones(0).type(), tutorial::Person::PHONE_TYPE_HOME);
  EXPECT_EQ(person->phones(1).number(), "555-5678");
  EXPECT_EQ(person->phones(1).type(), tutorial::Person::PHONE_TYPE_MOBILE);
}

TEST_F(ProtobufQuickTest, SerializeAndDeserializeAddressBook) {
  // Create original address book
  tutorial::AddressBook original_book;

  // Add first person
  tutorial::Person* person1 = original_book.add_people();
  person1->set_name("Alice Smith");
  person1->set_id(456);
  person1->set_email("alice@example.com");

  tutorial::Person::PhoneNumber* phone1 = person1->add_phones();
  phone1->set_number("555-9999");
  phone1->set_type(tutorial::Person::PHONE_TYPE_WORK);

  // Add second person
  tutorial::Person* person2 = original_book.add_people();
  person2->set_name("Bob Johnson");
  person2->set_id(789);
  // Note: email is optional, so we won't set it for person2

  tutorial::Person::PhoneNumber* phone2 = person2->add_phones();
  phone2->set_number("555-0000");
  phone2->set_type(tutorial::Person::PHONE_TYPE_MOBILE);

  // Serialize to string
  std::string serialized_data;
  ASSERT_TRUE(original_book.SerializeToString(&serialized_data));
  EXPECT_GT(serialized_data.size(), 0);

  // Deserialize from string
  tutorial::AddressBook deserialized_book;
  ASSERT_TRUE(deserialized_book.ParseFromString(serialized_data));

  // Verify deserialized data matches original
  EXPECT_EQ(deserialized_book.people_size(), 2);

  // Check first person
  const tutorial::Person& deser_person1 = deserialized_book.people(0);
  EXPECT_EQ(deser_person1.name(), "Alice Smith");
  EXPECT_EQ(deser_person1.id(), 456);
  EXPECT_EQ(deser_person1.email(), "alice@example.com");
  EXPECT_EQ(deser_person1.phones_size(), 1);
  EXPECT_EQ(deser_person1.phones(0).number(), "555-9999");
  EXPECT_EQ(deser_person1.phones(0).type(), tutorial::Person::PHONE_TYPE_WORK);

  // Check second person
  const tutorial::Person& deser_person2 = deserialized_book.people(1);
  EXPECT_EQ(deser_person2.name(), "Bob Johnson");
  EXPECT_EQ(deser_person2.id(), 789);
  EXPECT_FALSE(deser_person2.has_email());  // email was not set
  EXPECT_EQ(deser_person2.phones_size(), 1);
  EXPECT_EQ(deser_person2.phones(0).number(), "555-0000");
  EXPECT_EQ(deser_person2.phones(0).type(),
            tutorial::Person::PHONE_TYPE_MOBILE);
}
