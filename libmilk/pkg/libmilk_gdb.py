# -*- python -*-
import gdb
import itertools
import re

class LibmilkVarMapIterator:
	def __init__(self, val):
		self.val = val
		self.count = 0
		self.n_buckets = self.val['_M_element_count']
		if self.n_buckets == 0:
			self.node = False
		else:
			self.bucket = self.val['_M_buckets']
			self.node = self.bucket[0]
			self.update ()
	def __iter__ (self):
		return self
	def update (self):
		while self.node == 0:
			self.bucket = self.bucket + 1
			self.node = self.bucket[0]
		if self.count == self.n_buckets:
			self.node = False
		else:
			self.count = self.count + 1

	def next (self):
		if not self.node:
			raise StopIteration
		result = self.node.dereference()['_M_v']
		self.node = self.node.dereference()['_M_next']
		self.update ()
		#return ('%s(%s)'%(str(result['first']),str(result['second']['t'])),result['second'])
		#return (str(result['first']) + '(' + str(result['second']['t']) + ')',result['second'])
		return (str(result['first']),result['second'])

class LibmilkVarArrayIterator:
	def __init__(self, val):
		self.val = val
		self.item = self.val['_M_impl']['_M_start']
		self.finish = self.val['_M_impl']['_M_finish']
		self.count = 0
	def __iter__ (self):
		return self
	def next (self):
		if self.item < self.finish:
			elt = self.item.dereference()
			self.item = self.item + 1
			count = self.count
			self.count = self.count + 1
			return ('[%d]' % count,elt)
		raise StopIteration

class LibmilkNullIterator:
	def __init__(self):
		self.val = 0
	def __iter__ (self):
		return self
	def next (self):
		raise StopIteration

class LibmilkVarPrinter:
	def __init__(self, val):
		self.val = val
	def to_string(self):
		s_t = str(self.val['t']);
		if s_t == 'lyramilk::data::var::t_invalid':
			return 't_invalid'
		if s_t == 'lyramilk::data::var::t_user':
			return 't_user'
		if s_t == 'lyramilk::data::var::t_bin':
			return 't_bin:%s' % self.val['u']['p'].dereference()
		if s_t == 'lyramilk::data::var::t_str':
			return 't_str:%s' % self.val['u']['s'].dereference()
		if s_t == 'lyramilk::data::var::t_wstr':
			return 't_wstr:%s' % self.val['u']['w'].dereference()
		if s_t == 'lyramilk::data::var::t_bool':
			return 't_bool:%s' % self.val['u']['b']
		if s_t == 'lyramilk::data::var::t_int':
			return 't_int:%s' % self.val['u']['i8']
		if s_t == 'lyramilk::data::var::t_uint':
			return 't_uint:%s' % self.val['u']['u8']
		if s_t == 'lyramilk::data::var::t_double':
			return 't_double:%s' % self.val['u']['f8']
		if s_t == 'lyramilk::data::var::t_array':
			return 't_array'
		if s_t == 'lyramilk::data::var::t_map':
			return 't_map'
	def children(self):
		s_t = str(self.val['t']);
		if s_t == 'lyramilk::data::var::t_user':
			return LibmilkVarMapIterator(self.val['u']['o'].dereference())
		if s_t == 'lyramilk::data::var::t_map':
			return LibmilkVarMapIterator(self.val['u']['m'].dereference())
		if s_t == 'lyramilk::data::var::t_array':
			return LibmilkVarArrayIterator(self.val['u']['a'].dereference())
		return ()

def lookup_var(val):
	if str(val.type) == 'lyramilk::data::var':
		return LibmilkVarPrinter(val)
	if str(val.type) == 'lyramilk::data::var &':
		return LibmilkVarPrinter(val)
	return None

#gdb.pretty_printers.insert(0,lookup_var)
gdb.pretty_printers.append(lookup_var)

print '[gdb]load pretty_printers for lyramilk::data::var	'