//
// StatementImpl.h
//
// $Id: //poco/Main/Data/include/Poco/Data/StatementImpl.h#15 $
//
// Library: Data
// Package: DataCore
// Module:  StatementImpl
//
// Definition of the StatementImpl class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef Data_StatementImpl_INCLUDED
#define Data_StatementImpl_INCLUDED


#include "Poco/Data/Data.h"
#include "Poco/Data/AbstractBinding.h"
#include "Poco/Data/AbstractExtraction.h"
#include "Poco/Data/Range.h"
#include "Poco/Data/Bulk.h"
#include "Poco/Data/Column.h"
#include "Poco/Data/Extraction.h"
#include "Poco/Data/BulkExtraction.h"
#include "Poco/Data/SessionImpl.h"
#include "Poco/RefCountedObject.h"
#include "Poco/AutoPtr.h"
#include "Poco/String.h"
#include "Poco/Format.h"
#include "Poco/Exception.h"
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <sstream>


namespace Poco {
namespace Data {


class Data_API StatementImpl: public Poco::RefCountedObject
	/// StatementImpl interface that subclasses must implement to define database dependent query execution.
	///
	/// StatementImpl's are noncopyable.
{
public:
	enum State
	{
		ST_INITIALIZED,
		ST_COMPILED,
		ST_BOUND,
		ST_PAUSED,
		ST_DONE,
		ST_RESET
	};

	enum Storage
	{
		STORAGE_DEQUE_IMPL,
		STORAGE_VECTOR_IMPL,
		STORAGE_LIST_IMPL,
		STORAGE_UNKNOWN_IMPL
	};

	enum BulkType
	{
		BULK_UNDEFINED,     
			/// Bulk mode not defined yet.
		BULK_BINDING,       
			/// Binding in bulk mode.
			/// If extraction is present in the same statement, 
			/// it must also be bulk.
		BULK_EXTRACTION,    
			/// Extraction in bulk mode.
			/// If binding is present in the same statement, 
			/// it must also be bulk.
		BULK_FORBIDDEN,     
			/// Bulk forbidden. 
			/// Happens when the statement has already been 
			/// configured as non-bulk.
	};

	static const std::string DEQUE;
	static const std::string VECTOR;
	static const std::string LIST;
	static const std::string UNKNOWN;

	StatementImpl(SessionImpl& rSession);
		/// Creates the StatementImpl.

	virtual ~StatementImpl();
		/// Destroys the StatementImpl.

	template <typename T> 
	void add(const T& t)
		/// Appends SQL statement (fragments).
	{
		_ostr << t;
	}

	void addBinding(AbstractBinding* pBinding);
		/// Registers the Binding at the StatementImpl.

	void addExtract(AbstractExtraction* pExtraction);
		/// Registers objects used for extracting data at the StatementImpl.

	void setExtractionLimit(const Limit& extrLimit);
		/// Changes the extractionLimit to extrLimit. Per default no limit (EXTRACT_UNLIMITED) is set.

	std::string toString() const;
		/// Create a string version of the SQL statement.

	Poco::UInt32 execute();
		/// Executes a statement. Returns the number of rows extracted.

	void reset();
		/// Resets the statement, so that we can reuse all bindings and re-execute again.

	State getState() const;
		/// Returns the state of the Statement.

	void setStorage(Storage storage);
		/// Sets the storage type for this statement;

	void setStorage(const std::string& storage);
		/// Sets the storage type for this statement;

	Storage getStorage() const;
		/// Returns the storage type for this statement.

	std::size_t extractionCount() const;
		/// Returns the number of extraction storage buffers associated
		/// with the statement.

	std::size_t dataSetCount() const;
		/// Returns the number of data sets associated with the statement.

protected:
	virtual Poco::UInt32 columnsReturned() const = 0;
		/// Returns number of columns returned by query. 

	virtual const MetaColumn& metaColumn(Poco::UInt32 pos) const = 0;
		/// Returns column meta data.

	const MetaColumn& metaColumn(const std::string& name) const;
		/// Returns column meta data.

	virtual bool hasNext() = 0;
		/// Returns true if a call to next() will return data. 
		///
		/// Note that the implementation must support
		/// several consecutive calls to hasNext without data getting lost, 
		/// ie. hasNext(); hasNext(); next() must be equal to hasNext(); next();

	virtual Poco::UInt32 next() = 0;
		/// Retrieves the next row or set of rows from the resultset and
		/// returns the number of rows retreved.
		///
		/// Will throw, if the resultset is empty.
		/// Expects the statement to be compiled and bound.

	virtual bool canBind() const = 0;
		/// Returns if another bind is possible.

	virtual void compileImpl() = 0;
		/// Compiles the statement, doesn't bind yet.

	virtual void bindImpl() = 0;
		/// Binds parameters.

	virtual AbstractExtractor& extractor() = 0;
		/// Returns the concrete extractor used by the statement.

	const AbstractExtractionVec& extractions() const;
		/// Returns the extractions vector.

	virtual AbstractBinder& binder() = 0;
		/// Returns the concrete binder used by the statement.

	int columnsExtracted() const;
		/// Returns the number of columns that the extractors handle.

	const AbstractBindingVec& bindings() const;
		/// Returns the bindings.

	AbstractBindingVec& bindings();
		/// Returns the bindings.

	AbstractExtractionVec& extractions();
		/// Returns the extractions vector.

	void makeExtractors(Poco::UInt32 count);
		/// Determines the type of the internal extraction container and
		/// calls the extraction creation function (addInternalExtract)
		/// with appropriate data type and container type arguments.
		/// 
		/// This function is only called in cases when there is data 
		/// returned by query, but no data storage supplied by user.
		///
		/// The type of the internal container is determined in the
		/// following order:
		/// 1. If statement has the container type set, the type is used.
		/// 2. If statement does not have the container type set,
		///    session is queried for container type setting. If the
		///    session container type setting is found, it is used.
		/// 3. If neither session nor statement have the internal
		///    container type set, std::vector is used.
		///
		/// Supported internal extraction container types are:
		/// - std::vector (default)
		/// - std::deque
		/// - std::list

	SessionImpl& session();
		/// Rteurns session associated with this statement.

	void fixupBinding();
		/// Sets the AbstractBinder at the bindings.

	void resetBinding();
		/// Resets binding so it can be reused again.

	virtual bool isStoredProcedure() const;
		/// Returns true if the statement is stored procedure.
		/// Used as a help to determine whether to automatically create the
		/// internal extractions when no outside extraction is supplied.
		/// The reason for this function is to prevent unnecessary internal
		/// extraction creation in cases (behavior exhibited by some ODBC drivers) 
		/// when there is data available from the stored procedure call 
		/// statement execution but no external extraction is supplied (as is 
		/// usually the case when stored procedures are called). In such cases
		/// no storage is needed because output parameters serve as storage.
		/// At the Data framework level, this function always returns false.
		/// When connector-specific behavior is desired, it should be overriden 
		/// by the statement implementation.

	void fixupExtraction();
		/// Sets the AbstractExtractor at the extractors.

	Poco::UInt32 currentDataSet() const;
		/// Returns the current data set.

	Poco::UInt32 activateNextDataSet();
		/// Returns the next data set, or -1 if the last data set was reached.

	Poco::UInt32 getExtractionLimit();
		/// Returns the extraction limit value.

	const Limit& extractionLimit() const;
		/// Returns the extraction limit.

private:
	void compile();
		/// Compiles the statement, if not yet compiled.

	void bind();
		/// Binds the statement, if not yet bound.

	Poco::UInt32 executeWithLimit();
		/// Executes with an upper limit set.

	Poco::UInt32 executeWithoutLimit();
		/// Executes without an upper limit set.

	void resetExtraction();
		/// Resets extraction so it can be reused again.

	template <class C>
	InternalExtraction<C>* createExtract(const MetaColumn& mc)
	{
		C* pData = new C;
		Column<C>* pCol = new Column<C>(mc, pData);
		return new InternalExtraction<C>(*pData, pCol);
	}

	template <class C>
	InternalBulkExtraction<C>* createBulkExtract(const MetaColumn& mc)
	{
		C* pData = new C;
		Column<C>* pCol = new Column<C>(mc, pData);
		return new InternalBulkExtraction<C>(*pData, pCol, getExtractionLimit());
	}

	template <class T>
	void addInternalExtract(const MetaColumn& mc)
		/// Creates and adds the internal extraction.
		///
		/// The decision about internal extraction container is done 
		/// in a following way:
		///
		/// If this statement has _storage member set, that setting
		/// overrides the session setting for storage, otherwise the
		/// session setting is used.
		/// If neither this statement nor the session have the storage
		/// type set, std::deque is the default container type used.
	{
		std::string storage;
	
		switch (_storage)
		{
		case STORAGE_DEQUE_IMPL:  
			storage = DEQUE; break;
		case STORAGE_VECTOR_IMPL: 
			storage = VECTOR; break;
		case STORAGE_LIST_IMPL:   
			storage = LIST; break;
		case STORAGE_UNKNOWN_IMPL:
			storage = AnyCast<std::string>(session().getProperty("storage")); 
			break;
		}

		if (storage.empty()) storage = DEQUE;

		if (0 == icompare(DEQUE, storage))
		{
			if (!isBulkExtraction())
				addExtract(createExtract<std::deque<T> >(mc));
			else
				addExtract(createBulkExtract<std::deque<T> >(mc));
		}
		else if (0 == icompare(VECTOR, storage))
		{
			if (!isBulkExtraction())
				addExtract(createExtract<std::vector<T> >(mc));
			else
				addExtract(createBulkExtract<std::vector<T> >(mc));
		}
		else if (0 == icompare(LIST, storage))
		{
			if (!isBulkExtraction())
				addExtract(createExtract<std::list<T> >(mc));
			else
				addExtract(createBulkExtract<std::list<T> >(mc));
		}
	}

	bool isNull(std::size_t col, std::size_t row) const;
		/// Returns true if the value in [col, row] is null.
		
	void forbidBulk();
		/// Forbids bulk operations.

	void setBulkBinding();
		/// Sets the bulk binding flag.

	void setBulkExtraction(const Bulk& l);
		/// Sets the bulk extraction flag and extraction limit.
	
	void resetBulk();
		/// Resets the bulk extraction and binding flag.

	bool bulkBindingAllowed() const;
		/// Returns true if statement can be set to bind data in bulk.
		/// Once bulk binding is set for a statement, it can be
		/// neither altered nor mixed with non-bulk mode binding.

	bool bulkExtractionAllowed() const;
		/// Returns true if statement can be set to extract data in bulk.
		/// Once bulk extraction is set for a statement, it can be
		/// neither altered nor mixed with non-bulk mode extraction.

	bool isBulkBinding() const;
		/// Returns true if statement is set to bind data in bulk.

	bool isBulkExtraction() const;
		/// Returns true if statement is set to extract data in bulk.

	bool isBulkSupported() const;
		/// Returns true if connector and session support bulk operation.

	StatementImpl(const StatementImpl& stmt);
	StatementImpl& operator = (const StatementImpl& stmt);

	State                    _state;
	Limit                    _extrLimit;
	Poco::UInt32             _lowerLimit;
	int                      _columnsExtracted;
	SessionImpl&             _rSession;
	Storage                  _storage;
	std::ostringstream       _ostr;
	AbstractBindingVec       _bindings;
	AbstractExtractionVecVec _extractors;
	Poco::UInt32             _curDataSet;
	BulkType                 _bulkBinding;
	BulkType                 _bulkExtraction;

	friend class Statement; 
};


//
// inlines
//
inline void StatementImpl::addBinding(AbstractBinding* pBinding)
{
	poco_check_ptr (pBinding);

	_bindings.push_back(pBinding);
}


inline std::string StatementImpl::toString() const
{
	return _ostr.str();
}


inline const AbstractBindingVec& StatementImpl::bindings() const
{
	return _bindings;
}


inline AbstractBindingVec& StatementImpl::bindings()
{
	return _bindings;
}


inline const AbstractExtractionVec& StatementImpl::extractions() const
{
	poco_assert (_curDataSet < _extractors.size());
	return _extractors[_curDataSet];
}


inline AbstractExtractionVec& StatementImpl::extractions()
{
	poco_assert (_curDataSet < _extractors.size());
	return _extractors[_curDataSet];
}


inline int StatementImpl::columnsExtracted() const
{
	return _columnsExtracted;
}


inline StatementImpl::State StatementImpl::getState() const
{
	return _state;
}


inline SessionImpl& StatementImpl::session()
{
	return _rSession;
}


inline void StatementImpl::setStorage(Storage storage)
{
	_storage = storage;
}


inline StatementImpl::Storage StatementImpl::getStorage() const
{
	return _storage;
}


inline std::size_t StatementImpl::extractionCount() const
{
	return extractions().size();
}


inline std::size_t StatementImpl::dataSetCount() const
{
	return _extractors.size();
}


inline bool StatementImpl::isStoredProcedure() const
{
	return false;
}


inline bool StatementImpl::isNull(std::size_t col, std::size_t row) const
{
	try 
	{
		return extractions().at(col)->isNull(row);
	}catch (std::out_of_range& ex)
	{ 
		throw RangeException(ex.what()); 
	}
}


inline Poco::UInt32 StatementImpl::currentDataSet() const
{
	return _curDataSet;
}


inline Poco::UInt32 StatementImpl::getExtractionLimit()
{
	return _extrLimit.value();
}


inline const Limit& StatementImpl::extractionLimit() const
{
	return _extrLimit;
}


inline void StatementImpl::forbidBulk()
{
	_bulkBinding = BULK_FORBIDDEN;
	_bulkExtraction = BULK_FORBIDDEN;
}


inline void StatementImpl::setBulkBinding()
{
	_bulkBinding = BULK_BINDING;
}


inline bool StatementImpl::bulkBindingAllowed() const
{
	return BULK_UNDEFINED == _bulkBinding ||
		BULK_BINDING == _bulkBinding;
}


inline bool StatementImpl::bulkExtractionAllowed() const
{
	return BULK_UNDEFINED == _bulkExtraction ||
		BULK_EXTRACTION == _bulkExtraction;
}


inline bool StatementImpl::isBulkBinding() const
{
	return BULK_BINDING == _bulkBinding;
}


inline bool StatementImpl::isBulkExtraction() const
{
	return BULK_EXTRACTION == _bulkExtraction;
}

	
inline void StatementImpl::resetBulk()
{
	_bulkExtraction = BULK_UNDEFINED;
	_bulkBinding = BULK_UNDEFINED;\
	setExtractionLimit(Limit(Limit::LIMIT_UNLIMITED, false, false));
}


inline bool StatementImpl::isBulkSupported() const
{
	return _rSession.getFeature("bulk");
}


} } // namespace Poco::Data


#endif // Data_StatementImpl_INCLUDED
