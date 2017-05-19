#ifndef LIB_CIRC_BUF_H
#define LIB_CIRC_BUF_H

#include "Arduino.h"
#include "qpcpp.h"

class buffer {
public:
	buffer(byte *buf, uint16_t size) : buf(buf), size(size), head(buf), tail(buf), end(buf + size), count(0) {};
	~buffer() {};
	
	void push_back(byte item)
	{
		if(count > size){
			__BKPT();
			for(;;);
		}
		memcpy(head, &item, sizeof(byte));
		head = head + sizeof(byte);
		if(head == end)
		head = buf;
		count++;
	}

	byte pop_front()
	{
		byte ret;
		if(count == 0){
			__BKPT();
			for(;;);
		}
		memcpy(&ret, tail, sizeof(byte));
		tail = tail + sizeof(byte);
		if(tail == end){
			tail = buf;
		}
		count--;
		return ret;
	}
	
	void transfer_in(byte *data, int n){
		for(int i=0; i<n; i++) push_back(data[i]);
	}
	
	void transfer_out(byte *data, int n){
		for(int i=0; i<n; i++){
			byte d = pop_front();
			memcpy(data + i, &d, sizeof(byte));
		}
	}
	
	bool full(){ return (count == size); }
	bool empty(){ return (count == 0); }
	byte getCount(){ return count; }
	
private:
	uint16_t size;
	uint16_t count;
	byte *head;
	byte *tail;
	byte *end;
	
	byte *buf;
};

#endif