#ifndef DBAPI_H
#define DBAPI_H

#ifdef DLL_SPEC_EXPORTS
#define DLL_SPEC __declspec(dllexport)
#else
#define DLL_SPEC __declspec(dllimport)
#endif

#include <functional>
#include <windows.h>
#include <stdio.h>
#include <string> 
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>


class DLL_SPEC Object;
class DLL_SPEC Table;
class DLL_SPEC Db;

// Typ datov�ho pole
enum struct FieldType {
	Integer,
	Double,
	String,
	Field
};
// Polymorfn� datov� objekt (reprezentuje jednu datovou hodnotu v tabulce)
// Rozhran� vyhovuje z�kladn�m typ�m int, double, string; pro typ �field� je rozhran� roz���eno
class DLL_SPEC Object {
public:
	Object();
	virtual ~Object();

	// Gettery a settery podle typu
	// Jejich funkce je definov�na jen v p��pad�, �e aktu�ln� objekt je odpov�daj�c�ho typu
	// Automatick� konverze v z�kladn�m API nejsou vy�adov�ny

	virtual std::string getString() const;
	virtual void setString(std::string value);

	virtual int getInt() const;
	virtual void setInt(int value);

	virtual double getDouble() const;
	virtual void setDouble(double value);

	// Vrac� true, pokud aktu�ln� objekt p�edstavuje dan� typ
	virtual bool isType(FieldType type) const;
};


class DLL_SPEC IntObject : public Object {
public:
	IntObject() : value(0) {}
	IntObject(int v) : value(v) {}
	int getValue() const { return value; }
	void setValue(int value) { this->value = value; }
private:
	int value;
};

class DLL_SPEC DoubleObject : public Object {
public:
	DoubleObject() : value(0.0) {}
	DoubleObject(double v) : value(v) {}
	double getValue() const { return value; }
	void setValue(double value) { this->value = value; }
private:
	double value;
};

class DLL_SPEC StringObject : public Object {
public:
	StringObject() : value("") {}
	StringObject(std::string v) : value(v) {}
	std::string getValue() const { return value; }
	void setValue(std::string value) { this->value = value; }
private:
	std::string value;
};

// Objekt popisuj�c� sloupe�ek �field�
class DLL_SPEC FieldObject : public Object {
public:
	FieldObject() { name = ""; type = FieldType::Field; }
	FieldObject(std::string name, FieldType type) :name(name), type(type) {}

	virtual bool isType(FieldType type) const override
	{
		return type == this->type;
	}

	// N�zev sloupe�ku
	std::string getName() const { return name; }
	// Typ sloupe�ku
	FieldType getType() const { return type; }
private:
	std::string name;
	FieldType type;
};

class DLL_SPEC Db {
public:
	// Otev�e datab�zi
	static Db* open(std::string database);
	// Uzav�e datab�zi (dealokuje pam�ov� prost�edky)
	void close();

	// Vytvo�� novou tabulku
	Table* createTable(std::string name, int fieldsCount, FieldObject** fields);
	// Otev�e existuj�c� tabulku
	Table* openTable(std::string name);
	// Otev�e tabulku (pokud neexistuje, vytvo�� automaticky novou)
	Table* openOrCreateTable(std::string name, int fieldsCount, FieldObject** fields);

	// Alokuje objekt �int�
	static Object* Int(int value) { return new IntObject(value); }
	// Alokuje objekt �double�
	static Object* Double(double value) { return new DoubleObject(value); }
	// Alokuje objekt �string�
	static Object* String(std::string value) { return new StringObject(value); }
	// Alokuje objekt �field�
	static FieldObject* Field(std::string name, FieldType type) { return new FieldObject(name, type); }

	static const std::string dbLocation; //Cesta k datab�z�m staticky dan� -> databases/
	std::string getDatabaseName() const { return this->databaseName; }
	const std::vector<Table*> getTables() const { return tables; }
private:	
	std::string databaseName; //Db si uchov�v� svoje jm�no
	std::vector<Table*> tables; //Uklad�m si tabulky pro jejich dealokaci p�es close
	int tablesCount = 0;

};
// --------------------------------------------------------

// Rozhran� iter�toru (select)
class DLL_SPEC Iterator {
public:
	virtual ~Iterator() {}
	// Posun na dal�� ��dek (vrac� true, pokud je iter�tor platn�; logika podle Java Iterator)
	virtual bool moveNext() = 0;
	// Vrac� pole Object* obsahuj�c� data ��dku
	virtual Object** getRow() = 0;
	// Vrac� intern� rowId aktu�ln�ho ��dku
	virtual int getRowId() = 0;
	// Uzav�e iter�tor (dealokuje pam�ov� prost�edky)
	virtual void close() = 0;
};

// Tabulka
class DLL_SPEC Table {
public:
	Table(FieldObject** tableFields, int tableFieldsCount, std::string tableFilePath, std::string tableName)
	{
		this->tableFields = tableFields;
		this->tableFieldsCount = tableFieldsCount;
		this->rowCount = 0;
		this->tableFilePath = tableFilePath;
		this->tableName = tableName;
	} //Konstruktor pro vytvo�en� pr�zdn� tabulky

	Table(FieldObject** tableFields, int tableFieldsCount, std::string tableFilePath, std::string tableName, std::vector<Object**> data)
		: Table(tableFields, tableFieldsCount, tableFilePath, tableName)
	{
		this->data = data;
		this->rowCount = data.size();
	} //Konstruktor pro na�ten� dat tabulky

	// Vlo�en� nov�ho ��dku do tabulky (pole Object* (pro jednotliv� hodnoty sloupe�k�))
	void insert(Object** row);
	// Smaz�n� vyrabn�ho ��dku z tabulky
	void remove(int rowid);

	// Select � vytvo�� iter�tor k proch�zen� tabulky
	Iterator* select();

	// Commit � p�enese zm�ny z pam�ti do datov�ch soubor�
	void commit();

	// Uzav�e tabulku (dealokuje pam�ov� prost�edky)
	void close();

	// Vrac� po�et ��dk� v tabulce
	int getRowCount() const { return rowCount; }

	// Vrac� pole FieldObject* popisuj�c� sloupe�ky tabulky
	FieldObject** getFields() const { return tableFields; }

	// Vrac� po�et sloupe�k�
	int getFieldCount() const { return tableFieldsCount; }

	std::string getTableName() const { return tableName; }
private:
	int rowCount;
	int tableFieldsCount;
	FieldObject** tableFields;
	std::string tableFilePath; //Cesta k souboru pro commit
	std::string tableName;
	std::vector<Object**> data; //Vector ��dk�
};
#endif

