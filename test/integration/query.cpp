/*
 * query.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: ruben
 */

#include "mysql/connection.hpp"
#include <gmock/gmock.h>
#include <boost/asio/use_future.hpp>
#include "metadata_validator.hpp"
#include "integration_test_common.hpp"

namespace net = boost::asio;
using namespace testing;
using namespace mysql;

using mysql::detail::make_error_code;
using mysql::test::meta_validator;
using mysql::test::validate_meta;

namespace
{

struct QueryTest : public mysql::test::IntegTest
{
	using resultset_type = mysql::resultset<net::ip::tcp::socket>;

	QueryTest()
	{
		conn.handshake(connection_params);
	}

	void validate_eof(
		const resultset_type& result,
		int affected_rows=0,
		int warnings=0,
		int last_insert=0,
		std::string_view info=""
	)
	{
		EXPECT_TRUE(result.valid());
		EXPECT_TRUE(result.complete());
		EXPECT_EQ(result.affected_rows(), affected_rows);
		EXPECT_EQ(result.warning_count(), warnings);
		EXPECT_EQ(result.last_insert_id(), last_insert);
		EXPECT_EQ(result.info(), info);
	}

	void validate_2fields_meta(
		const resultset_type& result,
		const std::string& table
	) const
	{
		validate_meta(result.fields(), {
			meta_validator(table, "id", field_type::int_),
			meta_validator(table, "field_varchar", field_type::varchar, collation::utf8_general_ci)
		});
	}

};

template <typename... Types>
std::vector<mysql::value> makevalues(Types&&... args)
{
	return std::vector<mysql::value>{mysql::value(std::forward<Types>(args))...};
}

// Query, sync errc
TEST_F(QueryTest, QuerySyncErrc_InsertQueryOk)
{
	auto result = conn.query(
			"INSERT INTO inserts_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')", errc);
	ASSERT_EQ(errc, mysql::error_code());
	EXPECT_TRUE(result.fields().empty());
	EXPECT_TRUE(result.valid());
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(result.affected_rows(), 1);
	EXPECT_EQ(result.warning_count(), 0);
	EXPECT_GT(result.last_insert_id(), 0);
	EXPECT_EQ(result.info(), "");
}

TEST_F(QueryTest, QuerySyncErrc_InsertQueryFailed)
{
	auto result = conn.query(
			"INSERT INTO bad_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')", errc);
	ASSERT_EQ(errc, make_error_code(mysql::Error::no_such_table));
	EXPECT_FALSE(result.valid());
}

TEST_F(QueryTest, QuerySyncErrc_UpdateQueryOk)
{
	auto result = conn.query(
			"UPDATE updates_table SET field_int = field_int+1", errc);
	ASSERT_EQ(errc, mysql::error_code());
	EXPECT_TRUE(result.fields().empty());
	EXPECT_TRUE(result.valid());
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(result.affected_rows(), 2);
	EXPECT_EQ(result.warning_count(), 0);
	EXPECT_EQ(result.last_insert_id(), 0);
	EXPECT_THAT(std::string(result.info()), HasSubstr("Rows matched"));
}

TEST_F(QueryTest, QuerySyncErrc_SelectOk)
{
	auto result = conn.query("SELECT * FROM empty_table", errc);
	ASSERT_EQ(errc, mysql::error_code());
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	validate_2fields_meta(result, "empty_table");
}

TEST_F(QueryTest, QuerySyncErrc_SelectQueryFailed)
{
	auto result = conn.query("SELECT field_varchar, field_bad FROM one_row_table", errc);
	ASSERT_EQ(errc, make_error_code(mysql::Error::bad_field_error));
	EXPECT_FALSE(result.valid());
}

// Query, sync exc
TEST_F(QueryTest, QuerySyncExc_Ok)
{
	auto result = conn.query(
			"INSERT INTO inserts_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')");
	EXPECT_TRUE(result.fields().empty());
	EXPECT_TRUE(result.valid());
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(result.affected_rows(), 1);
	EXPECT_EQ(result.warning_count(), 0);
	EXPECT_GT(result.last_insert_id(), 0);
	EXPECT_EQ(result.info(), "");
}

TEST_F(QueryTest, QuerySyncExc_Error)
{
	EXPECT_THROW(
		conn.query("INSERT INTO bad_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')"),
		boost::system::system_error
	);
}

// Query, async
TEST_F(QueryTest, QueryAsync_InsertQueryOk)
{
	auto fut = conn.async_query(
		"INSERT INTO inserts_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')",
		net::use_future
	);
	ctx.run();
	auto result = fut.get();
	EXPECT_TRUE(result.fields().empty());
	EXPECT_TRUE(result.valid());
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(result.affected_rows(), 1);
	EXPECT_EQ(result.warning_count(), 0);
	EXPECT_GT(result.last_insert_id(), 0);
	EXPECT_EQ(result.info(), "");
}

TEST_F(QueryTest, QueryAsync_InsertQueryFailed)
{
	auto fut = conn.async_query(
		"INSERT INTO bad_table (field_varchar, field_date) VALUES ('v0', '2010-10-11')",
		net::use_future
	);
	ctx.run();
	validate_future_exception(fut, make_error_code(mysql::Error::no_such_table));
}

TEST_F(QueryTest, QueryAsync_UpdateQueryOk)
{
	auto fut = conn.async_query(
		"UPDATE updates_table SET field_int = field_int+1",
		net::use_future
	);
	ctx.run();
	auto result = fut.get();
	EXPECT_TRUE(result.fields().empty());
	EXPECT_TRUE(result.valid());
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(result.affected_rows(), 2);
	EXPECT_EQ(result.warning_count(), 0);
	EXPECT_EQ(result.last_insert_id(), 0);
	EXPECT_THAT(std::string(result.info()), HasSubstr("Rows matched"));
}

TEST_F(QueryTest, QueryAsync_SelectOk)
{
	auto fut = conn.async_query("SELECT * FROM empty_table", net::use_future);
	ctx.run();
	auto result = fut.get();
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	validate_2fields_meta(result, "empty_table");
}

TEST_F(QueryTest, QueryAsync_SelectQueryFailed)
{
	auto fut = conn.async_query("SELECT field_varchar, field_bad FROM one_row_table", net::use_future);
	ctx.run();
	validate_future_exception(fut, make_error_code(Error::bad_field_error));
}


// Fetch
TEST_F(QueryTest, FetchOneSyncErrc_SelectOkNoResults)
{
	auto result = conn.query("SELECT * FROM empty_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Already in the end of the resultset, we receive the EOF
	const mysql::row* row = result.fetch_one(errc);
	EXPECT_EQ(errc, mysql::error_code());
	EXPECT_EQ(row, nullptr);
	validate_2fields_meta(result, "empty_table");
	validate_eof(result);

	// Fetching again just returns null
	row = result.fetch_one(errc);
	EXPECT_EQ(errc, mysql::error_code());
	EXPECT_EQ(row, nullptr);
	validate_eof(result);
}

TEST_F(QueryTest, FetchOneSyncErrc_SelectOkOneRow)
{
	auto result = conn.query("SELECT * FROM one_row_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Fetch only row
	const mysql::row* row = result.fetch_one(errc);
	ASSERT_EQ(errc, mysql::error_code());
	ASSERT_NE(row, nullptr);
	validate_2fields_meta(result, "one_row_table");
	EXPECT_EQ(row->values(), makevalues(1, "f0"));
	EXPECT_FALSE(result.complete());

	// Fetch next: end of resultset
	row = result.fetch_one(errc);
	ASSERT_EQ(errc, mysql::error_code());
	ASSERT_EQ(row, nullptr);
	validate_eof(result);
}

TEST_F(QueryTest, FetchOneSyncErrc_SelectOkTwoRows)
{
	auto result = conn.query("SELECT * FROM two_rows_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Fetch first row
	const mysql::row* row = result.fetch_one(errc);
	ASSERT_EQ(errc, mysql::error_code());
	ASSERT_NE(row, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row->values(), makevalues(1, "f0"));
	EXPECT_FALSE(result.complete());

	// Fetch next row
	row = result.fetch_one(errc);
	ASSERT_EQ(errc, mysql::error_code());
	ASSERT_NE(row, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row->values(), makevalues(2, "f1"));
	EXPECT_FALSE(result.complete());

	// Fetch next: end of resultset
	row = result.fetch_one(errc);
	ASSERT_EQ(errc, mysql::error_code());
	ASSERT_EQ(row, nullptr);
	validate_eof(result);
}

// There seems to be no real case where fetch can fail (other than net fails)

TEST_F(QueryTest, FetchOneSyncExc_SelectOkTwoRows)
{
	auto result = conn.query("SELECT * FROM two_rows_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Fetch first row
	const mysql::row* row = result.fetch_one();
	ASSERT_NE(row, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row->values(), makevalues(1, "f0"));
	EXPECT_FALSE(result.complete());

	// Fetch next row
	row = result.fetch_one();
	ASSERT_NE(row, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row->values(), makevalues(2, "f1"));
	EXPECT_FALSE(result.complete());

	// Fetch next: end of resultset
	row = result.fetch_one();
	ASSERT_EQ(row, nullptr);
	validate_eof(result);
}


// Query for INT types





}





