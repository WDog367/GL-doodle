/**
* section: Tree
* synopsis: Navigates a tree to print element names
* purpose: Parse a file to a tree, use xmlDocGetRootElement() to
*          get the root element, then walk the document and print
*          all the element name in document order.
* usage: tree1 filename_or_URL
* test: tree1 test2.xml > tree1.tmp && diff tree1.tmp $(srcdir)/tree1.res
* author: Dodji Seketeli
* copy: see Copyright for the status of this software.
*/
#include <stdio.h>
//#include <libxml/parser.h>
//#include <libxml/tree.h>
#include <iostream>
#include <tinyxml2.h>

/*
*To compile this file using gcc you can type
*gcc `xml2-config --cflags --libs` -o xmlexample libxml2-example.c
*/

/**
* print_element_names:
* @a_node: the initial xml node to consider.
*
* Prints the names of the all the xml elements
* that are siblings or children of a given xml node.
*/
static void
print_element_names(tinyxml2::XMLElement * node)
{
	static int depth = 0;
	tinyxml2::XMLElement *curNode = node;
	
	depth++;
	for (; curNode; curNode = curNode->NextSiblingElement()) {
		for (int i = 0; i < depth - 1; i++) {
			std::cout << "  ";
		}
		std::cout << curNode->Name() << std::endl;
		print_element_names(curNode->FirstChildElement());
	}
	depth--;
}

/**
* Simple example to parse a file called "file.xml",
* walk down the DOM, and print the name of the
* xml elements nodes.
*/
int
main(int argc, char **argv)
{
	using namespace tinyxml2;

	const char filename[25] = "skeletonTest.dae";
	XMLElement *curNode;
	XMLElement *rootNode;
	XMLDocument doc;
	doc.LoadFile(filename);

	rootNode = doc.FirstChildElement();

	print_element_names(rootNode);

	return 0;
}