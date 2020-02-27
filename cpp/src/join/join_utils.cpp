#include <glog/logging.h>
#include "join_utils.hpp"
#include "../util/arrow_utils.hpp"

namespace twisterx {
  namespace join {
	namespace util {
	  arrow::Status build_final_table(const std::shared_ptr<std::map<int64_t,
																	 std::vector<int64_t >>> &joined_indices,
									  const std::shared_ptr<arrow::Table> &left_tab,
									  const std::shared_ptr<arrow::Table> &right_tab,
									  std::shared_ptr<arrow::Table> *final_table,
									  arrow::MemoryPool *memory_pool) {
		std::vector<int64_t> left_indices;
		std::vector<int64_t> right_indices;
		auto it = joined_indices->begin();
		while (it != joined_indices->end()) {
		  auto left_index = it->first;
		  for (int64_t &right_index : it->second) {
			left_indices.push_back(left_index);
			right_indices.push_back(right_index);
		  }
		  it++;
		}

		// creating joined schema
		std::vector<std::shared_ptr<arrow::Field>> fields;
		fields.insert(fields.end(), left_tab->schema()->fields().begin(), left_tab->schema()->fields().end());
		fields.insert(fields.end(), right_tab->schema()->fields().begin(), right_tab->schema()->fields().end());
		auto schema = arrow::schema(fields);

		std::vector<std::shared_ptr<arrow::Array>> data_arrays;

		// build arrays for left tab
		for (auto &column :left_tab->columns()) {
		  std::shared_ptr<arrow::Array> destination_col_array;
		  arrow::Status
			  status = twisterx::util::copy_array_by_indices(std::make_shared<std::vector<int64_t >>(left_indices),
															 column->chunk(0),
															 &destination_col_array,
															 memory_pool);
		  if (status != arrow::Status::OK()) {
			LOG(FATAL) << "Failed while copying a column to the final table from left table. " << status.ToString();
			return status;
		  }
		  data_arrays.push_back(destination_col_array);
		}

		// build arrays for right tab
		for (auto &column :right_tab->columns()) {
		  std::shared_ptr<arrow::Array> destination_col_array;
		  arrow::Status
			  status = twisterx::util::copy_array_by_indices(std::make_shared<std::vector<int64_t >>(right_indices),
															 column->chunk(0),
															 &destination_col_array,
															 memory_pool);
		  if (status != arrow::Status::OK()) {
			LOG(FATAL) << "Failed while copying a column to the final table from right table. " << status.ToString();
			return status;
		  }
		  data_arrays.push_back(destination_col_array);
		}
		*final_table = arrow::Table::Make(schema, data_arrays);
		return arrow::Status::OK();
	  }
	}
  }
}