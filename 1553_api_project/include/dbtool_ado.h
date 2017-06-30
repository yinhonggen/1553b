#if !defined(ADO_WRAPPER_DBTOOL_API_H)
#define ADO_WRAPPER_DBTOOL_API_H
/**
 * ADO_WRAPPER ADO接口的包装库，提供一个更简单的、面向C++编程人员的接口，把ADO的细节和复杂性隐藏起来，
 * 同时对于BusRobot项目提供一个简单的、基于配置文件的(数据库)连接管理功能。
 *
 * 此接口提供3个类Connection、PreparedStatement、ResultSet：
 *
 * Connection
 * 实现连接管理，同时用户可以使用它进行事务管理。
 *
 * PreparedStatement
 * 封装SQL语句的执行接口，可以处理动态SQL语句，通过C++类型的参数直接传递数据。
 *
 * ResultSet
 * SQL语句返回的结果集，可以方便的(以C++类型)获取每行数据。
 *
 * @author anhu.xie 2008-11-06
 */
// 原计划实现部分做成一个DLL，所以有以下宏定义
#ifdef DBTOOL_EXPORTS
#define DBTOOL_API __declspec(dllexport)
#else
#define DBTOOL_API __declspec(dllimport)
#endif
// 目前不是DLL，是源码级共享，可直接引用此.h文件。
// BTW，需要Connection::Connection(void)的请把dbTool.cpp放到自己的工程。
#undef DBTOOL_API
#define DBTOOL_API

#include <string>
#include <map>
#import "msado15.dll" rename("EOF", "AdoEOF")
#include <oledberr.h>

// 接口部分
namespace ADO_WRAPPER {

	class ResultSet;
	class PreparedStatement;
	class Connection;
	using std::string;

	/**
	 * 数据库连接
	 */
	class DBTOOL_API Connection {
		friend PreparedStatement;
		ADODB::_ConnectionPtr conn_ado; // 所包装的ADO连接
		bool created_new_conn; // 连接是否是我们自己创建的？
		void CreateConnection(const char *conn_str, const char *usr, const char *passwd);
	public:
		/**
		 * 根据配置文件创建默认数据库连接
		 * 先不提供！
		 */
//		Connection();
		/**
		 * 使用指定的连接参数创建数据库连接
		 * @param conn_str 连接字符串
		 * @param usr 登录数据库的用户名
		 * @param passwd 登录数据库的密码
		 */
		Connection(const char *conn_str, const char *usr, const char *passwd);
		/**
		 * 使用已有的ADO Connection对象管理数据库连接
		 * @param pconn 已有的ADO连接，虽然是一个IDispatch指针，但必须是ADO Connection对象
		 */
		Connection(IDispatch * pconn);
		/**
		 * 拷贝构造函数，追踪我们自己打开的连接，并保证关闭
		 */
		Connection(const Connection &src);
		/**
		 * 析构函数，负责关闭我们自己打开的连接
		 */
		~Connection();
		/**
		 * 打开一个新的数据库连接
		 * @param conn_str ADO连接字符串
		 * @param usr 登录数据库用的用户名
		 * @param passwd 登录数据库用的密码
		 */
		void open(const char *conn_str, const char *usr, const char *passwd) {
			conn_ado = NULL;
			conn_ado.CreateInstance(__uuidof(ADODB::Connection));
			CreateConnection(conn_str, usr, passwd);
		}
		/**
		 * 创建一个执行SQL语句的对象
		 * @return 执行SQL语句的PreparedStatement对象
		 */
		PreparedStatement CreateStatement();
		/**
		 * 创建一个执行SQL语句的对象，并且准备传入的SQL语句
		 * @param sql_stmt_string 要准备执行的SQL语句
		 * @return 执行SQL语句的PreparedStatement对象，并且其语句已经准备好，可以直接设置参数和运行了
		 */
		PreparedStatement prepare_statement(const char *sql_stmt_string);
		/**
		 * 事务管理：开始一个事务
		 * @return 对于支持多级事务的驱动程序，返回当前事务的级别
		 */
		long begin_trans() { return conn_ado->BeginTrans(); }
		/**
		 * 事务管理：提交(确认)事务
		 * @return 对于支持多级事务的驱动程序，返回当前事务的级别
		 */
		long commit_trans() { return conn_ado->CommitTrans(); }
		/**
		 * 事务管理：撤消事务
		 * @return 对于支持多级事务的驱动程序，返回当前事务的级别
		 */
		long rollback_trans() { return conn_ado->RollbackTrans(); }

		long close() { return conn_ado->Close(); }


		IDispatch* GetDispatch() { return conn_ado; }

		/**
		* 获取ADO的Error接口
		* 该接口可以获取sql错误码，解决_com_error类不能得到ADO SQLState的问题
		*/
		ADODB::ErrorsPtr GetErrors() { return conn_ado->GetErrors(); }
	};

	/**
	 * 执行数据库SQL的语句
	 */
	class DBTOOL_API PreparedStatement {
		Connection conn; // 数据库连接
		ADODB::_CommandPtr stmt; // 准备好的SQL语句
		ADODB::ParametersPtr para; // 语句需要设置的参数
		// (oledb)Provider不支持参数信息导出时，必须手动创建并设置参数
		std::map<long, ADODB::_ParameterPtr> man_para; // 手动设置的参数集
		bool params_refresh; // 是否支持参数信息导出？
		void set_param_value(long index, _variant_t value, size_t data_len);
	public:
		/**
		 * 构造函数：创建语句对象
		 * @param conn 数据库连接对象。注意我们使用的是一个对象，而不是一个指针(为了与前期代码兼容)。
		 */
		PreparedStatement(Connection conn);
		/**
		 * 析构函数：清理工作
		 */
		~PreparedStatement();
		/**
		 * 直接执行(没有Prepare)一个SQL语句。
		 * 对于没有参数需要设置，一次性执行的语句，使用这个方法。
		 * @param sqlString 要执行的语句。
		 * @return 如果是查询(SELECT)语句，返回一个结果集；
		 * 如果是数据更新(INSERT/UPDATE)语句，不返回结果集，请丢弃返回的对象。
		 */
		ResultSet execute(const char *sqlString);
		/**
		 * 准备(Prepare)一个SQL语句。
		 * 对于需要设置参数(动态SQL)，或者需要反复执行的语句，可以先调用Prepare(const char*)，
		 * 然后(反复)调用set<Type>(long,<type>)与Execute()执行SQL语句。
		 * @param sqlString 要准备的语句。
		 * @return 如果是查询(SELECT)语句，返回一个结果集；
		 * 如果是数据更新(INSERT/UPDATE)语句，不返回结果集，请丢弃返回的对象。
		 */
		void prepare(const char *sqlString);
		/**
		 * 设置操作超时时间,若不设置，ADO默认的是30s
		 * \param timeout 超时值，单位是s，0表示无限长（直到操作结束或出错）
		 */
		void set_timeout( long timeout );
		/**
		 * 设置已准备好的动态SQL语句的参数为一个short类型的值
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_short(long index, short value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个long类型的值
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_long(long index, long value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个bigInt类型的值
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_bigInt(long index, __int64 value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个float类型的浮点数
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_float(long index, float value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个double类型的浮点数
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_double(long index, double value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个bool类型的逻辑值
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void set_bool(long index, bool value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个字符串
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值(C++类型的字符串)
		 */
		void set_string(long index, string value) { set_string(index,value.c_str()); }
		/**
		 * 设置已准备好的动态SQL语句的参数为一个字符串
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值(C类型的字符串)
		 */
		void set_string(long index, const char *value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个字符串
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值(COM类型的字符串)
		 */
		void set_string(long index, const _bstr_t &value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个十进制数(DECIMAL)
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void setDecimal(long index, const DECIMAL &value);
		/**
		 * 设置已准备好的动态SQL语句的参数为一个COM类型
		 * @param index 参数的位置，从0开始计数
		 * @param value 参数的取值
		 */
		void setVariant(long index, const _variant_t &value);
		/**
		 * 设置已准备好的动态SQL语句参数值为一个二进制数据(字节串)
		 * @param index 参数的位置，从0开始计数
		 * @param data 数据字节串的位置
		 * @param data_len 数据的长度(字节数)
		 */
		void set_bytes(long index, const char *data, size_t data_len);
		/**
		 * 设置已准备好的动态SQL语句参数值为一个二进制数据(字节串)
		 * 这是一个C++调用，方便设置固定长度的数据。
		 * @param index 参数的位置，从0开始计数
		 * @param data 字符数组
		 * @tparam data_len 数据的长度(字节数)
		 */
		template<size_t data_len>
		void set_bytes(long index, const unsigned char (&data)[data_len]) {
			set_bytes(index, reinterpret_cast<const char *>(data), data_len);
		}
		/**
		 * 设置已准备好的动态SQL语句参数的值为空(NULL)值
		 * @param index 参数的位置，从0开始计数
		 */
		void set_null(long index);
		/**
		 * 执行一个已准备好(并且设置了合适的参数)的SQL语句。
		 * 对于需要设置参数(动态SQL)，或者需要反复执行的语句，可以先调用Prepare(const char*)，
		 * 然后(反复)调用set<Type>(long,<type>)与Execute()执行SQL语句。
		 * 对于动态SQL语句，在调用Execute()之前，必须每个参数都被设置。
		 * @return 如果是查询(SELECT)语句，返回一个结果集；
		 * 如果是数据更新(INSERT/UPDATE)语句，不返回结果集，请丢弃返回的对象。
		 */
		ResultSet execute();
		/**
		* 执行一个已准备好(并且设置了合适的参数)的SQL语句。
		* 不返回结果集
		*/
		void execute_noRecords();

		ResultSet executeAsync();

		void cancel();

	};

	/**
	 * SQL查询语句返回的结果集。
	 * 注意我们叫ResultSet而不是(ADO的)RecordSet，因为我们只用它来取查询的结果，而不能用它来修改数据库内容。
	 */
	class DBTOOL_API ResultSet {
		friend ResultSet PreparedStatement::execute();
		friend ResultSet PreparedStatement::execute(const char *);
		bool lastFieldGetWasNull; // 最后所取列(字段)值是否为NULL
		ADODB::_RecordsetPtr rs; // 所包装的ADO记录集
		ADODB::FieldsPtr flds; // 结果集的列(字段)
		/**
		 * 构造函数：创建对象。
		 * 被定义为private，是为了禁止用户自己创建。结果集对象只能由PreparedStatement对象返回。
		 */
		ResultSet(ADODB::_RecordsetPtr r) : rs(r), flds(r->Fields), lastFieldGetWasNull(true) {}
	public:
		/**
		 * 析构函数：资源清理工作。
		 */
		~ResultSet() { rs = NULL; flds = NULL; }
		/**
		 * 是否已到数据集合的末尾。
		 * @return 如果返回true，表示已到结果集末尾，没有有效的数据行可用；
		 * 如果返回false，则上一个操作(MoveNext()或者打开结果集操作)取得了有效的数据行。
		 */
		bool db_eof() { return rs->AdoEOF != 0; }
		/**
		 * 结果是否已到数据集合顶端。
		 * @return 如果返回true，表示已到结果集顶端，上一个MovePrev()没有取得有效的数据行；
		 * 如果返回false，标识上一个MovePrev()操作取得了有效的数据行
		 */
		bool db_bof() { return rs->BOF != 0; }
		/**
		 * 关闭结果集。
		 * 从资源管理的角度，这个方法的调用很重要。如果用户已经不再使用一个结果集，应该关闭它。
		 */
		void close() {
			if ( rs->GetState() == ADODB::adStateOpen )
				rs->Close();
		}
		/**
		 * 结果集浏览：正序(从前往后移动)访问下一行数据。
		 * move_next()之后，应该调用DBEOF()来判读是否取得有效的数据。
		 */
		void move_next() { rs->MoveNext(); }
		/**
		 * 结果集浏览：逆序(从后往前移动)访问下一行数据。
		 * MovePrev()之后，应该调用BOF()来判读是否取得有效的数据。
		 */
		void move_prev() { rs->MovePrevious(); }
		/**
		 * 结果集浏览：逆序(从后往前移动)访问第一行数据。
		 * MoveFirst()之后，应该调用BOF()来判读是否取得有效的数据。
		 */
		void move_first() { rs->MoveFirst(); }
		/**
		 * 结果集浏览：正序(从前往后移动)访问最后一行数据。
		 * MoveLastt()之后，应该调用DBEOF()来判读是否取得有效的数据。
		 */
		void move_last() { rs->MoveLast(); }
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为short类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 */
		short get_short(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为long类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		long get_long(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为bigint类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		__int64 get_bigInt(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为float类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		float get_float(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为double类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		double get_double(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为bool类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		bool get_bool(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为DECIMAL类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		DECIMAL getDecimal(long index);
		/**
		 * 从当前行获取数据字段(列值)，并把取得的值转换为COM字符串类型。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值，对于可空的列，需要紧跟着调用wasNull()来判断是否为NULL。
		 */
		string get_string(long index);
		/**
		 * 从当前行获取数据字段(列值)，并且用COM的通用类型返回。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @return 取得的列值。
		 * 对于可空的列，可以紧跟着调用wasNull()来判断是否为NULL。
		 */
		_variant_t getVariant(long index);
		/**
		 * 从当前行获取数据字段(列值)，这是把列值当成一个二进制数据(字节串)来获取。
		 * 注意，这不是转换，如果数据库里定义的列不是二进制数据，会引发异常。
		 * @param index 要访问的列在结果集中位置，从0开始计数。
		 * @param buffer 存放结果。其内存由getBytes()分配，由调用者负责释放(用delete[])。
		 * @return 取得的二进制数据的字节数。
		 * 对于可空的列，可以紧跟着调用wasNull()来判断是否为NULL。
		 */
		size_t get_bytes(long index, char *&buffer);
		/**
		 * 获取指定长度的二进制数据(字节串)
		 * 这是一个C++调用，方便用固定固定长度豁出去获得数据。
		 * @param index 参数的位置，从0开始计数
		 * @param data 字符数组
		 * @tparam data_len 数据的长度(字节数)
		 */
		template<size_t data_len>
		size_t get_bytes(long index, unsigned char (&data)[data_len]);
		/**
		 * 最后一次调用get<Type>(long...)方法返回的列值是否为空(NULL)值？
		 * 注意是wasNull()而不是isNull()，所以一定要先调用get<Type>(long...)方法。
		 * @return true标识最后返回列的值为NULL，false标识最后返回列值有效。
		 * 对于可空的列，可以紧跟着调用wasNull()来判断是否为NULL。
		 */
		bool was_null() { return lastFieldGetWasNull; }
		/**
		 * 结果集的列数
		 * @return 返回结果集中字段(列)的数目。
		 */
		long col_count() { return flds->Count; }
	};
}

//================================================================
// 以下为实现。不必再往下看了;-)
inline ADO_WRAPPER::Connection::Connection(const char *conn_str, const char *usr, const char *passwd)
	: conn_ado("ADODB.Connection"), created_new_conn(false)
{
	CreateConnection(conn_str, usr, passwd);
}
inline ADO_WRAPPER::Connection::Connection(IDispatch * Connection) : created_new_conn(false)
{
	if ( Connection == NULL )
		conn_ado = NULL;
	else {
		HRESULT hr = Connection->QueryInterface(&conn_ado);
		if ( FAILED(hr) )
			_com_issue_error(hr);
		if ( Connection == NULL )
			_com_issue_error(E_INVALIDARG);
	}
}
inline ADO_WRAPPER::Connection::Connection(const Connection &src)
	: conn_ado(src.conn_ado), created_new_conn(false)
{
}
inline ADO_WRAPPER::Connection::~Connection()
{
	if ( created_new_conn )
		conn_ado->Close();
}
inline void ADO_WRAPPER::Connection::CreateConnection(const char *c, const char *u, const char *p)
{
	created_new_conn = false;
	HRESULT hr = conn_ado->Open(c, u, p, 0);
	if ( FAILED(hr) )
		_com_issue_error(hr);
	else
		created_new_conn = true;
	//使用客户端游标
	//默认情况是服务器端游标：adUseServer，该游标不支持MovePrevious等操作。
//	hr = conn_ado->put_CursorLocation(ADODB::adUseClient);
// 	if ( FAILED(hr) )
// 		_com_issue_error(hr);
}

//==============
inline ADO_WRAPPER::PreparedStatement ADO_WRAPPER::Connection::CreateStatement()
{
	return PreparedStatement(*this);
}
inline ADO_WRAPPER::PreparedStatement ADO_WRAPPER::Connection::prepare_statement(const char *sqlstr)
{
	PreparedStatement st(*this);
	st.prepare(sqlstr);
	return st;
}

inline ADO_WRAPPER::PreparedStatement::PreparedStatement(Connection dbc)
	: conn(dbc), stmt("ADODB.Command"), params_refresh(false)
{
}
inline ADO_WRAPPER::PreparedStatement::~PreparedStatement()
{
	conn = NULL;
	stmt = NULL;
	para = NULL;
}

inline void ADO_WRAPPER::PreparedStatement::prepare(const char *sql) {
	stmt->ActiveConnection = conn.conn_ado;
	stmt->CommandText = sql;
	stmt->Prepared = true;
	para = stmt->Parameters;
	params_refresh = false;
	man_para.clear();
	if ( para != NULL )
		try {
			para->Refresh();
			params_refresh = true;
	}
	catch ( _com_error &e) {
		if ( e.Error() != DB_E_PARAMUNAVAILABLE )
			throw;
	}
}

inline void ADO_WRAPPER::PreparedStatement::set_param_value(long index, _variant_t value, size_t data_len) {
	if ( params_refresh )
		para->Item[index]->Value = value;
	else {
		ADODB::_ParameterPtr cp = stmt->CreateParameter("", ADODB::adVarBinary, ADODB::adParamInput, data_len, value);
		man_para[index] = cp;
	}
}
inline void ADO_WRAPPER::PreparedStatement::set_timeout(long sec) {
	stmt->PutCommandTimeout( sec );
}

inline void ADO_WRAPPER::PreparedStatement::cancel() {
	
	stmt->Cancel();

}

inline ADO_WRAPPER::ResultSet ADO_WRAPPER::PreparedStatement::executeAsync() {
	if ( !params_refresh )
		for (std::map<long, ADODB::_ParameterPtr>::iterator i = man_para.begin(); i != man_para.end(); ++i )
			para->Append(i->second);
	ADODB::_RecordsetPtr rs = stmt->Execute(NULL,NULL,ADODB::adCmdText | ADODB::adAsyncExecute);
//	return ResultSet(rs);
}

inline void ADO_WRAPPER::PreparedStatement::execute_noRecords() {
	if ( !params_refresh )
		for (std::map<long, ADODB::_ParameterPtr>::iterator i = man_para.begin(); i != man_para.end(); ++i )
			para->Append(i->second);
	stmt->Execute(NULL,NULL,ADODB::adCmdText | ADODB::adExecuteNoRecords);

}
inline ADO_WRAPPER::ResultSet ADO_WRAPPER::PreparedStatement::execute() {
	if ( !params_refresh )
		for (std::map<long, ADODB::_ParameterPtr>::iterator i = man_para.begin(); i != man_para.end(); ++i )
			para->Append(i->second);
	ADODB::_RecordsetPtr rs = stmt->Execute(NULL,NULL,ADODB::adCmdText);
	return ResultSet(rs);
}
inline ADO_WRAPPER::ResultSet ADO_WRAPPER::PreparedStatement::execute(const char *sql) {
	stmt->ActiveConnection = conn.conn_ado;
	stmt->CommandText = sql;
	ADODB::_RecordsetPtr rs = stmt->Execute(NULL,NULL,ADODB::adCmdText);
	return ResultSet(rs);
}
inline void ADO_WRAPPER::PreparedStatement::set_short(long index, short v) {
	set_param_value(index, v, sizeof v);
}
inline void ADO_WRAPPER::PreparedStatement::set_long(long index, long v) {
	set_param_value(index, v, sizeof v);
}
inline void ADO_WRAPPER::PreparedStatement::set_bigInt(long index, __int64 v) {
	// VC < 7不能直接处理__int64:(借用DECIMAL
#if _MSC_VER < 0x1300
	DECIMAL dec;
	dec.signscale = 0;
	dec.Hi32 = 0;
	dec.Lo64 = v;
	set_param_value(index, dec, sizeof v);
#else
	set_param_value(index, v, sizeof v);
#endif
}
inline void ADO_WRAPPER::PreparedStatement::set_float(long index, float v) {
	set_param_value(index, v, sizeof v);
}
inline void ADO_WRAPPER::PreparedStatement::set_double(long index, double v) {
	set_param_value(index, v, sizeof v);
}
inline void ADO_WRAPPER::PreparedStatement::set_bool(long index, bool v) {
	set_param_value(index, (v != 0), sizeof v);
}
inline void ADO_WRAPPER::PreparedStatement::set_string(long index, const char *v) {
	set_param_value(index, v, strlen(v));
}
inline void ADO_WRAPPER::PreparedStatement::set_string(long index, const _bstr_t &v) {
	set_param_value(index, v, v.length());
}
inline void ADO_WRAPPER::PreparedStatement::setVariant(long index, const _variant_t &v) {
	if ( v.vt == VT_EMPTY )
		set_null(index);
	else {
		size_t len = 8;
		switch ( v.vt ) {
		case VT_BSTR:
		case VT_VARIANT:
		case VT_LPSTR:
		case VT_LPWSTR:
			len = v.operator _bstr_t().length();
			break;
		case VT_ARRAY:
		case VT_SAFEARRAY:
			len = v.parray->rgsabound[0].cElements;
			break;
		}
		set_param_value(index, v, len);
	}
}
inline void ADO_WRAPPER::PreparedStatement::set_null(long index) {
	_variant_t vNull;
	vNull.vt = VT_NULL;
	set_param_value(index, vNull, 0);
}

inline void ADO_WRAPPER::PreparedStatement::setDecimal(long index, const DECIMAL &v) {
	set_param_value(index, v, sizeof v);
}

inline void ADO_WRAPPER::PreparedStatement::set_bytes(long index, const char *data, size_t data_len) {
	SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, data_len);
	if ( ! psa )
		_com_issue_error(E_POINTER);
	BYTE *dst = NULL;
	SafeArrayAccessData(psa, (void**)&dst);
	memcpy(dst, data, data_len);
	SafeArrayUnaccessData(psa);

	_variant_t v;
	v.vt = VT_ARRAY | VT_UI1;
	v.parray = psa;
	set_param_value(index, v, data_len);
}


//==================
inline __int64 ADO_WRAPPER::ResultSet::get_bigInt(long index) {
	const _variant_t &v = getVariant(index);
	// VC < 7不能直接处理__int64:(借用DECIMAL
#if _MSC_VER < 1300
	return v.operator DECIMAL().Lo64;
#else
	return v;
#endif
}
inline long ADO_WRAPPER::ResultSet::get_long(long index) {
	return getVariant(index);
}
inline short ADO_WRAPPER::ResultSet::get_short(long index) {
	return getVariant(index);
}
inline std::string ADO_WRAPPER::ResultSet::get_string(long index) {
	return (char *)(_bstr_t)getVariant(index);
}
inline double ADO_WRAPPER::ResultSet::get_double(long index) {
	return getVariant(index);
}
inline float ADO_WRAPPER::ResultSet::get_float(long index) {
	return getVariant(index);
}
inline bool ADO_WRAPPER::ResultSet::get_bool(long index) {
	return getVariant(index);
}
inline DECIMAL ADO_WRAPPER::ResultSet::getDecimal(long index) {
	return getVariant(index);
}
inline _variant_t ADO_WRAPPER::ResultSet::getVariant(long index) {
	const _variant_t &v = flds->Item[index]->Value;
	lastFieldGetWasNull = (v.vt == VT_NULL);
	return lastFieldGetWasNull ? _variant_t() : v;
}
inline size_t ADO_WRAPPER::ResultSet::get_bytes(long index, char *&dst) {
	const _variant_t &v = flds->Item[index]->Value;
	lastFieldGetWasNull = (v.vt == VT_NULL);
	if ( v.vt == VT_NULL || v.vt == VT_EMPTY )
		return 0;
	if ( v.vt != (VT_ARRAY | VT_UI1) )
		_com_issue_error(E_INVALIDARG);
	SAFEARRAY *psa = v.parray;
	VARTYPE vt;
	HRESULT hr = SafeArrayGetVartype(psa, &vt);
	if ( FAILED(hr) )
		_com_issue_error(hr);
	if ( vt != VT_UI1 || SafeArrayGetDim(psa) != 1 )
		_com_issue_error(E_INVALIDARG);
	long len = psa->rgsabound[0].cElements;
	dst = new char[len];
	char *src = NULL;
	hr = SafeArrayAccessData(psa, (void**)&src);
	if ( FAILED(hr) )
		_com_issue_error(hr);
	memcpy(dst, src, len);
	SafeArrayUnaccessData(psa);
	return len;
}
template<size_t data_len>
size_t ADO_WRAPPER::ResultSet::get_bytes(long index, unsigned char (&data)[data_len]) {
	const _variant_t &v = flds->Item[index]->Value;
	lastFieldGetWasNull = (v.vt == VT_NULL);
	if ( v.vt == VT_NULL || v.vt == VT_EMPTY )
		return 0;
	if ( v.vt != (VT_ARRAY | VT_UI1) )
		_com_issue_error(E_INVALIDARG);
	SAFEARRAY *psa = v.parray;
	VARTYPE vt;
	HRESULT hr = SafeArrayGetVartype(psa, &vt);
	if ( FAILED(hr) )
		_com_issue_error(hr);
	if ( vt != VT_UI1 || SafeArrayGetDim(psa) != 1 )
		_com_issue_error(E_INVALIDARG);
	long len = psa->rgsabound[0].cElements;
	char *src = NULL;
	hr = SafeArrayAccessData(psa, (void**)&src);
	if ( FAILED(hr) )
		_com_issue_error(hr);
	memmove(data, src, len > data_len ? data_len : len);
	SafeArrayUnaccessData(psa);
	return len;
}

#endif
