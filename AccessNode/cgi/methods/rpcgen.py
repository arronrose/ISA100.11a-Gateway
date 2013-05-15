#! /usr/bin/python

import getopt, sys
import json
import re
import os

class Schema:
	def __init__(self, _n, _s, _i):
		self.json={}
		self.name = _n
		self.namespace=_s
		self.includes=_i
	def load(self):
		print "\t[",self.namespace, self.name,self.includes,"]"
		fname=""
		try:
			if not os.path.exists(self.name):
				if not os.path.exists(self.includes+"/"+self.name):
					print "Failed to open file [", self.includes,"/",self.name,"]"
					sys.exit(-1)
				else:
					fname=self.includes+"/"+self.name
			else:
				fname=self.name
			s = open( fname, 'r').read()
			self.json = json.loads( s )
		except IOError:
			print "Failed to open file [",fname,"]"
			sys.exit(-1)

def usage():
	print "Usage: rpcgen.py [OPTIONS] [JSON_FILES]"
	print "OPTIONS:"
	print "\t-h|--help\t\tPrint this message"
	print "\t-a|--append\t\tAppend output."
	print "\t-I|--include\t\tWhere to search for included methods.json"
	print "\t-o|--output\t\tOutput file."
	print "\t-A|--acl\t\tGenerate MethodsACL.h files"
	print "\t-g|--gperf\t\tGenerate method.gperf files"
	print "\t-H|--methods.h\t\tGenerate header files"
	print "\t-L|--methods.html\t\tGenerate HTML files"
	print "\t-M|--mergeschemas\tMerge the json schemas into the output."
	print "\t-n|--namespaces\t\tSchema namespaces."
	sys.exit(1)

def unique(s):
	"""Return a list of the elements in s, but without duplicates.

	For example, unique([1,2,3,1,2,3]) is some permutation of [1,2,3],
	unique("abcabc") some permutation of ["a", "b", "c"], and
	unique(([1, 2], [2, 3], [1, 2])) some permutation of
	[[2, 3], [1, 2]].

	For best speed, all sequence elements should be hashable.  Then
	unique() will usually work in linear time.

	If not possible, the sequence elements should enjoy a total
	ordering, and if list(s).sort() doesn't raise TypeError it's
	assumed that they do enjoy a total ordering.  Then unique() will
	usually work in O(N*log2(N)) time.

	If that's not possible either, the sequence elements must support
	equality-testing.  Then unique() will usually work in quadratic
	time.
	"""

	n = len(s)
	if n == 0:
		return []

	# Try using a dict first, as that's the fastest and will usually
	# work.  If it doesn't work, it will usually fail quickly, so it
	# usually doesn't cost much to *try* it.  It requires that all the
	# sequence elements be hashable, and support equality comparison.
	u = {}
	try:
		for x in s:
			u[x] = 1
	except TypeError:
		del u  # move on to the next method
	else:
		return u.keys()

	# We can't hash all the elements.  Second fastest is to sort,
	# which brings the equal elements together; then duplicates are
	# easy to weed out in a single pass.
	# NOTE:  Python's list.sort() was designed to be efficient in the
	# presence of many duplicate elements.  This isn't true of all
	# sort functions in all languages or libraries, so this approach
	# is more effective in Python than it may be elsewhere.
	try:
		t = list(s)
		t.sort()
	except TypeError:
		del t  # move on to the next method
	else:
		assert n > 0
		last = t[0]
		lasti = i = 1
		while i < n:
			if t[i] != last:
				t[lasti] = last = t[i]
				lasti += 1
			i += 1
		return t[:lasti]

	# Brute force is all that's left.
	u = []
	for x in s:
		if x not in u:
			u.append(x)
	return u


def aclLine(methodName):
	words = methodName.split('.')
	out=''
	for word in words:
		out += word[0].upper() + word[1:]
	out+='ACL'
	return out

def methodID(methodName):
	p = re.compile('\.')
	a = p.subn('_',methodName)
	return a[0].upper()


# CIsa100::MhUnmarshaller::GetInstance
# CIsa100Unmarshaller::GetInstance
def methodMarshalClass(methodName):
	words = methodName.split('.')

	out = 'C'+words[0][0].upper()+words[0][1:]
	last = len(words)-1

	for i in range(1,last):
		if i != last:
			out+='::'
		out +=words[i][0].upper()+words[i][1:]

	out += 'Unmarshaller::GetInstance'

	return out


#line += '"\"'+methodName+'\"'+', reinterpret_cast<int (MethodUnmarshaller::*)( RPCCommand& cmd )>(&CConfigUnmarshaller::getConfig),
def reinterpretCastLine(methodName):
	#replaces any dot with ::
	words = methodName.split('.')

	out='reinterpret_cast<int (MethodUnmarshaller::*)( RPCCommand& cmd )>(&C'+words[0][0].upper()+words[0][1:]

	last = len(words)-1
	for i in range(1,last):
		if i != last:
			out+='::'
		out +=words[i][0].upper()+words[i][1:]

	out += 'Unmarshaller::'+words[last] +')'
	return out

def mergeSchemas(jsonSchemas):
	outSchema = {}
	outSchema['schemas']=[]
	outSchema['json']={}
	newSchemas=outSchema

	for it in jsonSchemas:
		outSchema['schemas'].append(it)
		outSchema['json'].update( it.json )

	namespaces=[]
	includedFiles=[]
	for fname in outSchema['schemas']:
		if 'include' in fname.json:
			includedFiles.extend( fname.json['include'] )
			del fname.json['include']
			for include in includedFiles:
				dir=include.split('/')
				if len(dir) <= 1:
					continue
				print "DIR", dir[0]
				newSchemas = mergeSchemas( fillSchemas(includedFiles,dir[0],fname.includes) )
	outSchema['schemas'].extend(newSchemas['schemas'])
	outSchema['json'].update(newSchemas['json'])
	if 'include' in outSchema['json']:
		del outSchema['json']['include']
	return outSchema

#******************************************************************************
#******************************************************************************
###############################################################################
def generateMergedSchema(jsonSchemas, output, mode):
	print " * MERGE schemas"
	outSchema = mergeSchemas(jsonSchemas)

	out = open( output, mode)
	out.write( json.dumps(outSchema['json'],sort_keys=True, indent=8) )
	out.close()

###############################################################################
def generateMethodsH(jsonSchemas, output, mode ):
	print " * GEN Headers:",output

	result = mergeSchemas(jsonSchemas)
	names=[]

	out = open(output,mode)

	out.write("#ifndef _"+result['schemas'][0].namespace.upper()+"_METHODS_H_\n")
	out.write("#define _"+result['schemas'][0].namespace.upper()+"_METHODS_H_\n\n")
	for fname in result['schemas']:
		for methodName in fname.json.keys():
			if methodName == "description":
				continue
			n = methodName.split(".")[0]
			if not n in names:
				names.append(n)
				out.write( '#include "methods/'+fname.namespace+'/'+n.title()+'Impl.h"\n' )
				out.write( '#include "methods/'+fname.namespace+'/'+n.title()+'Unmarshaller.h"\n\n' )
	out.write( '#endif\t/* '+fname.namespace.upper()+'_METHODS_H_ */\n' )
	out.close()
	print "done"

###############################################################################

def generateMethodsHtml(jsonSchemas, output, mode, namespaces):
	print " * GEN HTML",output

	result = mergeSchemas(jsonSchemas)
	d= result['json'].keys().sort()

	out = open(output,mode)
	out.write('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">\n')
	out.write('<HTML>\n')
	out.write('<HEAD>\n')
	out.write('<META HTTP-EQUIV="CONTENT-TYPE" CONTENT="text/html; charset=utf-8">\n')
	out.write('	<TITLE></TITLE>\n')
	out.write('</HEAD>\n')
	out.write('<BODY>\n')
	out.write('<table BORDER="1">\n')
	out.write('\t<tr>\n')
	out.write('\t\t<th>MethodName</th>\n')
	out.write('\t\t<th>ReturnValue</th>\n')
	out.write('\t\t<th>Parameters</th>\n')
	out.write('\t</tr>\n')

	for fname in result['schemas']:
		for methodName in fname.json.keys():
			if methodName == "description":
				continue
			out.write( '\t<tr>\n')
			out.write( '\t\t<td>'+methodName+'</td>\n')
			if not "return" in fname.json[methodName]:
				out.write('\t</tr>')
				continue
			out.write( '\t\t<td>'+json.dumps(fname.json[methodName]['return'])+'</td>\n')
			out.write( '\t\t<td>\n')
			out.write( '\t\t\t<table width="100%" border="1" cellspacing="1">\n')
			out.write( '\t\t\t<tr>\n')
			out.write( '\t\t\t\t<th style="width:20%">Name</th>\n')
			out.write( '\t\t\t\t<th>Type</th>\n')
			out.write( '\t\t\t\t<th>Optional</th>\n')
			out.write( '\t\t\t\t<th style="width:70%">Description</th>\n')
			out.write( '\t\t\t</tr>\n')

			if not 'params' in fname.json[methodName] or fname.json[methodName]['params']==None:
				print methodName,":no parameters"
				out.write( '\t\t\t</tr></table>\n')
				out.write( '\t\t</td>\n')
				continue
			params=fname.json[methodName]['params']
			for p in params:
				out.write( '\t\t\t<tr>\n')
				out.write( '\t\t\t\t<td><p>'+p+'</p></td>\n')
				if type(params[p]) is dict:
					out.write( '\t\t\t\t<td><p>'+params[p]['type']+'</p></td>\n')
					if 'optional' in params[p] and params[p]['optional']==True:
						out.write( '\t\t\t\t<td><p>true</p></td>\n')
					else:
						out.write( '\t\t\t\t<td><p>false</p></td>\n')
					if not 'description' in params[p]:
						print "warning: no description for",methodName
						out.write( '\t\t\t\t<td><p></p></td>\n')
					else:
						out.write( '\t\t\t\t<td><p>'+params[p]['description']+'</p></td>\n')
				else:
					out.write( '\t\t\t\t<td><p></p></td>\n')
					out.write( '\t\t\t\t<td><p></p></td>\n')
					out.write( '\t\t\t\t<td><p></p></td>\n')
				out.write( '\t\t\t</tr>\n')
			out.write( '\t\t\t</table>\n')
			out.write( '\t\t</td>\n')

		out.write('</table></body>\n')


def generateTopMethodsH(jsonSchemas, output, mode, namespaces):
	print " * GEN Headers:",output

	result = mergeSchemas(jsonSchemas)
	#d= result['json'].keys().sort()

	out = open(output,mode)
	out.write('#ifndef _JSON_RPC_METHODS_H_\n')
	out.write('#define _JSON_RPC_METHODS_H_\n')
	out.write('#include <cstdio>\n')
	out.write('#include "JsonRPC.h"\n')
	out.write('enum METHOD_ID {\n\tNO_LOGIN=1,\n')

	for methodName in result['json']:
		if methodName == "description":
			continue
		# replace the dots with undelines, while uppercasing the methodname
		#n = methodName.split(".")
		n = methodName.upper().replace('.','_')
		out.write('\t'+n+',\n')
		#out.write( '\t'+n[0].upper()+'_'+n[1].upper()+',\n')

	out.write('\tMAX_METHODS\n};\n')
	out.write('class MethodUnmarshaller {\n')
	out.write('public:\n')
	out.write('        MethodUnmarshaller() {}\n')
	out.write('        ~MethodUnmarshaller() {}\n')
	out.write('};\n')
	for name in namespaces.split(' '):
		out.write('#include "methods/'+name+'/methods.h"\n')
	out.write('#endif\n')






def generateMethodsACLH(jsonSchemas, output, mode):
	print " * GEN ACL:",output
	result = mergeSchemas(jsonSchemas)

	out = open(output,mode)
	out.write('#ifndef _METHODS_ACL_H_\n')
	out.write('#define _METHODS_ACL_H_\n')
	out.write('#include "methods/UserSet.h"\n')

	for methodName in result['json']:
		if methodName == "description":
			continue
		aclline = aclLine(methodName)
		out.write( "unsigned int "+ aclline +" [] = { ")
		for user in result['json'][methodName]['extra']['users']:
			out.write(user)
		out.write(' } ;\n')
	out.write('#endif\t/* _METHODS_ACL_H_ */\n')

def generateMethodsGperf(jsonSchemas, output, mode):
	print " * GEN GPERF:",output

	result = mergeSchemas(jsonSchemas)
	out = open(output,mode)
	out.write('%{\n')
	out.write('#include "methods/MethodsACL.h"\n')
	out.write('#include "methods/methods.h"\n')
	out.write('typedef struct JsonRPCMethodCode JsonRPCMethodCode;\n')
	out.write('%}\n')
	out.write('struct JsonRPCMethodOption\n')
	out.write('{\n')
	out.write('	const char	*MethodName;\n')
	out.write('	int		(MethodUnmarshaller::*unmarshall)(RPCCommand& cmd);\n')
	out.write('	MethodUnmarshaller* (*GetInstance)(void);\n')
	out.write('	unsigned int	id;\n')
	out.write('	unsigned int	*ace;\n')
	out.write('	unsigned int	nbAce;\n')
	out.write('};\n')
	out.write('%%\n')

	for methodName in result['json']:
		line=""
		if methodName == "description":
			continue

		print >>out, '"\\"%s\\"", %s, %s, %s, %s, %d' % (methodName
		, reinterpretCastLine(methodName)
		, methodMarshalClass(methodName)
		, methodID(methodName)
		, aclLine(methodName)
		, len(result['json'][methodName]['extra']['users']) )
		if 'aliases' in result['json'][methodName]:
			for alias in result['json'][methodName]['aliases']:
				print >>out, '"\\"%s\\"", %s, %s, %s, %s, %d' % ( alias
				, reinterpretCastLine(methodName)
				, methodMarshalClass(methodName)
				, methodID(methodName)
				, aclLine(methodName)
				, len(result['json'][methodName]['extra']['users']) )
	out.close()

def fillSchemas(schemaFiles, namespace, includes):
	ret=[]
	for schema in schemaFiles:
		o = Schema(schema,namespace, includes)
		o.load()
		ret.append( o )
	return ret

def main():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "aAhHLgTo:Mn:i:", ["help", "topmethods.h", "methods.html", "append", "acl", "gperf", "output=", "methods.h", "namespaces=", "merge","include="])
	except getopt.GetoptError, err:
		# print help information and exit:
		print str(err) # will print something like "option -a not recognized"
		usage()

	output = None
	verbose = False
	generate= ""
	mode = "w"
	namespaces=""
	acl = False
	merge = False
	gperf = False
	includes=""

	if len(sys.argv) == 1:
		usage()


	for o, a in opts:
		if o == "-v":
			verbose = True
		elif o in ("-h", "--help"):
			usage()
			sys.exit()
		elif o in ("-H", "--methods.h"):
			generate = 'methods.h'
		elif o in ("-T", "--topmethods.h"):
			generate = 'topmethods.h'
		elif o in ("-L", "--methods.html"):
			generate = 'methods.html'
		elif o in ("-n", "--namespaces"):
			namespaces = a
		elif o in ("-g", "--gperf"):
			gperf=True
		elif o in ("-M", "--merge"):
			merge = True
		elif o in ("-a", "--append"):
			mode = "a"
		elif o in ("-A", "--acl"):
			acl = True
		elif o in ("-o", "--output"):
			output = a
		elif o in ("-I", "--include"):
			includes = a
		else:
			assert False, "unhandled option"


	if generate == 'methods.h':
		generateMethodsH( fillSchemas(args[0:], namespaces,includes), output, mode)
	if generate == 'topmethods.h':
		generateTopMethodsH(fillSchemas(args[0:],namespaces,includes), output, mode, namespaces)
	if generate == 'methods.html':
		generateMethodsHtml(fillSchemas(args[0:],namespaces,includes), output, mode, namespaces)
	if merge == True:
		print ">>>",includes
		generateMergedSchema(fillSchemas(args[0:],"a", includes), output, "w")
	if acl == True:
		generateMethodsACLH(fillSchemas(args[0:],"", includes), output, mode)
	if gperf == True:
		generateMethodsGperf(fillSchemas(args[0:],"", includes), output, mode)

if __name__ == "__main__":
	main()

