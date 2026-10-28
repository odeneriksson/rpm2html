/*
 * rdf_api.h : implementation of methods to read and write RDF schemas
 *
 * See Copyright for the status of this software.
 *
 * $Id: rdf_api.c,v 1.30 2010/10/05 14:36:54 hany Exp $
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rpm2html.h"
#include "rpmdata.h"
#include "rdf_api.h"
/*
 * this module has been converted to libxml2, you need libxml2-devel or
 * older to compile it.
 */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/parserInternals.h>

/*
 * Reading an RDF schema stored in a file, this just build the
 * document tree, and check that it's an RDF:RDf element.
 */
rdfSchema rdfRead(const char *filename) {
    xmlDocPtr res = NULL;
    int ret;
    xmlParserCtxtPtr ctxt;
    xmlSAXHandler silent, *old;
    xmlNodePtr root;

    /*
     * Do a silent call to the XML parser.
     */
    ctxt = xmlCreateFileParserCtxt(filename);
    if (ctxt == NULL) return(NULL);
    memcpy(&silent, ctxt->sax, sizeof(silent));
    old = ctxt->sax;
    silent.error = NULL;
    silent.warning = NULL;
    silent.fatalError = NULL;
    ctxt->sax = &silent;

    xmlParseDocument(ctxt);

    ret = ctxt->wellFormed;
    res = ctxt->myDoc;
    ctxt->sax = old;
    xmlFreeParserCtxt(ctxt);

    /*
     * This is not well formed.
     */
    if (!ret) {
	xmlFreeDoc(res);
	fprintf(stderr, "rdfRead %s: resource is not wellformed XML\n",
		filename);
	return(NULL);
    }

    root = xmlDocGetRootElement(res);
    if (root == NULL) {
	fprintf(stderr, "rdfRead %s: tree is empty\n", filename);
        xmlFreeDoc(res);
	return(NULL);
    }
    if (!xmlStrEqual(root->name, BAD_CAST "RDF")) {
	fprintf(stderr, "rdfRead %s: not an RDF file\n", filename);
        xmlFreeDoc(res);
	return(NULL);
    }
    return((rdfSchema) res);
}

/*
 * Writing an RDF schema to a file, this just save the
 * document tree as an XML document.
 */
void rdfWrite(rdfSchema rdf, const char *filename) {
    FILE *output;
    
    output = fopen(filename, "w");
    if (output == NULL) {
        fprintf(stderr, "rdfWrite : couldn't save to file %s: %s\n",
	        filename, strerror(errno));
        return;
    }
    xmlDocFormatDump(output, (xmlDocPtr) rdf, 1);
    fclose(output);
}

/*
 * Writing an RDF schema to a memory buffer, this just save the
 * document tree as an XML document.
 */
void rdfWriteMemory(rdfSchema rdf, char **buffer, int *size) {
    xmlDocDumpMemory((xmlDocPtr) rdf, (xmlChar **) buffer, size);
}

/*
 * Creating an RDF schema tree in memory.
 */
rdfSchema rdfNewSchema() {
    xmlDocPtr res;
    xmlNsPtr rdf;
   
    res = xmlNewDoc(BAD_CAST "1.0");
    res->xmlRootNode = xmlNewDocNode(res, NULL, BAD_CAST "RDF", NULL);
    rdf = xmlNewNs(res->xmlRootNode, BAD_CAST "http://www.w3.org/TR/WD-rdf-syntax#", BAD_CAST "RDF");
    rdf = xmlNewNs(res->xmlRootNode, BAD_CAST "http://www.w3.org/1999/02/22-rdf-syntax-ns#", BAD_CAST "rdf");
    xmlSetNs(res->xmlRootNode, rdf);

    return((rdfSchema) res);
}

/*
 * Adding a new namespace to an RDF schema.
 */
rdfNamespace rdfNewNamespace(rdfSchema rdf, const char *url,
                             const char *namespace) {
    xmlNodePtr root;
    xmlNsPtr ns = NULL;
   
    root = xmlDocGetRootElement(rdf);
    if (root != NULL)
	ns = xmlNewNs(root, BAD_CAST url, BAD_CAST namespace);

    return((rdfNamespace) ns);
}

/*
 * Get a namespace associated to an RDF schema.
 * Should be stored either as an old NS on the document or as local
 * namespace stored on the root element.
 */
rdfNamespace rdfGetNamespace(rdfSchema rdf, const char *href) {
    xmlNodePtr root;

    root = xmlDocGetRootElement(rdf);
    return((rdfNamespace) xmlSearchNsByHref(rdf, root, BAD_CAST href));
}

/*
 * Get the RDF namespace associated to an RDF schema.
 */
rdfNamespace rdfGetRdfNamespace(rdfSchema rdf) {
    rdfNamespace ns;
   
    ns = rdfGetNamespace(rdf, "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
    if (ns != NULL)
        return((rdfNamespace) ns);
    ns = rdfGetNamespace(rdf, "http://www.w3.org/TR/WD-rdf-syntax#");
    if (ns != NULL)
        return((rdfNamespace) ns);
    /* create it ! */
    return(rdfNewNamespace(rdf, "http://www.w3.org/1999/02/22-rdf-syntax-ns#", "rdf"));
}


/*
 * Destroying an RDF schema in memory.
 */
void rdfDestroySchema(rdfSchema rdf) {
    xmlFreeDoc((xmlDocPtr) rdf);
}

/**
 ** An RDF schema is a collection of RDF descriptions.
 ** The following are used to browse the list.
 **/

/* First item */
rdfDescription rdfFirstDescription(rdfSchema schema) {
    xmlNodePtr tree;
    rdfNamespace rdfNs;

    rdfNs = rdfGetRdfNamespace(schema);
    tree = xmlDocGetRootElement(schema);
    if (tree == NULL) return(NULL);
    tree = tree->xmlChildrenNode;
    while (tree != NULL) {
        if ((tree->ns == rdfNs) && (xmlStrEqual(tree->name, BAD_CAST "Description")))
	    return(tree);
	tree = tree->next;
    }
    return(NULL);
}

/* Iteration : next item */
rdfDescription rdfNextDescription(rdfDescription desc) {
    rdfNamespace rdfNs;

    if (desc == NULL) return(NULL);
    rdfNs = desc->ns;
    desc = desc->next;
    while (desc != NULL) {
        if ((desc->ns == rdfNs) && (xmlStrEqual(desc->name, BAD_CAST "Description")))
	    return(desc);
	desc = desc->next;
    }
    return(NULL);
}

/*
 * Add an RDF description to an RDF schema, it just link it
 * the top level node of the description and requires further
 * action to fill it with the pertinent information.
 */
rdfDescription rdfAddDescription(rdfSchema schema, const char *id,
                                 const char *about) {
    xmlNodePtr tree;
    xmlNodePtr root;

    root = xmlDocGetRootElement(schema);
    tree = xmlNewChild(root, NULL, BAD_CAST "Description", NULL);
    if (id != NULL) {
        xmlNewProp(tree, BAD_CAST "ID", BAD_CAST id);
    }
    if (about != NULL) {
        xmlNewProp(tree, BAD_CAST "about", BAD_CAST about);
    }
    return((rdfDescription) tree);
}

/*
 * Read the attributes of a Description node.
 */
char *rdfGetDescriptionId(rdfSchema schema, rdfDescription desc) {
    char *res;

    res = (char *) xmlGetProp(desc, BAD_CAST "ID");
    return(res);
}

char *rdfGetDescriptionAbout(rdfSchema schema, rdfDescription desc) {
    char *res;
    rdfNamespace ns;
    char *buf;

    res = (char *) xmlGetProp(desc, BAD_CAST "about");
    if (res != NULL) return(res);

    /*
     * Maintain compatibility with old "RDF:HREF"
     */
    res = (char *) xmlGetProp(desc, BAD_CAST "HREF");
    if (res != NULL) return(res);
    ns = rdfGetRdfNamespace(schema);
    buf = (char *) xmlMalloc(xmlStrlen(ns->prefix) + 10);
    sprintf(buf, "%s:HREF", ns->prefix);
    res = (char *) xmlGetProp(desc, BAD_CAST buf);
    xmlFree(buf);
    return(res);
}

/*
 * Read any resource associated to an element.
 */
char *rdfGetElementResource(rdfSchema schema, rdfElement elem) {
    char *res;

    res = (char *) xmlGetProp(elem, BAD_CAST "resource");
    return(res);
}

void rdfSetElementResource(rdfSchema schema, rdfElement elem, const char *URI) {
    xmlSetProp(elem, BAD_CAST "resource", BAD_CAST URI);
}

/**
 ** Routines to read/write final values.
 **/

/*
 * browse the various property pertaining to this description,
 * and return the value which can be either a text (RDF_LEAF)
 * or an rdfElement (RDF_BAG, RDF_SEQ, RDF_ALT, RDF_DESC).
 * Having both reflect an invalid RDF document (while valid from
 * XML point of view).
 * This routine tries to ajust in case a request is done with or
 * or without a ns present or not on the node, and if the names
 * matches.
 *  e.g. a request for Name may return BIB:Name
 *    or a request for BIB:Name may return Name
 * if the exact match is not found.
 * 
 * Return -1 if not found
 *         0 if a single value is found
 *   RDF_xxx if a node is found.
 */
int rdfGetValue(rdfDescription desc, const char *property, 
                rdfNamespace ns, char **value, rdfElement *elem){
    rdfElement list = desc->xmlChildrenNode;
    rdfElement best = NULL;
    int approx = 0;

    if (value != NULL) *value = NULL;
    if (elem != NULL) *elem = NULL;

    while (list != NULL) {
        if ((list->ns == ns) && (xmlStrEqual(list->name, BAD_CAST property))) {
	    /*
	     * Found a perfect match, return it !
	     */
	    if (value != NULL) {
		*value = (char *) xmlNodeListGetString(desc->doc, list->xmlChildrenNode, 1);
            }
	    if (elem != NULL) {
		*elem = list->xmlChildrenNode;
		while ((*elem != NULL) && (xmlIsBlankNode(*elem)))
		    *elem = (*elem)->next;
	    }
	    if (xmlStrEqual(list->name, BAD_CAST "Bag")) return(RDF_BAG);
	    if (xmlStrEqual(list->name, BAD_CAST "Seq")) return(RDF_SEQ);
	    if (xmlStrEqual(list->name, BAD_CAST "Alt")) return(RDF_ALT);
	    if (xmlStrEqual(list->name, BAD_CAST "Description")) return(RDF_DESC);
	    return(0);
	}
	else if (((list->ns == NULL) || (ns == NULL)) &&
	         (xmlStrEqual(list->name, BAD_CAST property))) {
	    /*
	     * If more than one "best" match exists, ignore them !
	     */
	    if (approx) {
	        best = NULL;
	    } else {
                best = list;
		approx = 1;
            }
        }
	list = list->next;
    }
    if (best != NULL) {
	if (value != NULL) {
	    *value = (char *) xmlNodeListGetString(desc->doc, best->xmlChildrenNode, 1);
	}
	if (elem != NULL) *elem = best->xmlChildrenNode;
	if (xmlStrEqual(best->name, BAD_CAST "Bag")) return(RDF_BAG);
	if (xmlStrEqual(best->name, BAD_CAST "Seq")) return(RDF_SEQ);
	if (xmlStrEqual(best->name, BAD_CAST "Alt")) return(RDF_ALT);
	if (xmlStrEqual(best->name, BAD_CAST "Description")) return(RDF_DESC);
	return(0);
    }
    return(-1);
}

/*
 * browse the various property pertaining to this description,
 * update the value if found on a text mode, otherwise append
 * a new property at the end of the list.
 * Note that contrary to GetValue, the check is absolute !
 */
void rdfSetValue(rdfDescription desc, const char *property,
                 rdfNamespace ns, const char *value) {
    xmlChar *tmp;
    rdfElement list = desc->xmlChildrenNode;

    /*
     * Search the property.
     */
    while (list != NULL) {
        if ((list->ns == ns) && (xmlStrEqual(list->name, BAD_CAST property))) {
	    /*
	     * Property was found, update it !
	     */
	    if (list->xmlChildrenNode != NULL)
		xmlFreeNodeList(list->xmlChildrenNode);
	    tmp = xmlEncodeEntitiesReentrant(desc->doc, BAD_CAST value);
	    list->xmlChildrenNode = xmlStringGetNodeList(desc->doc, tmp);
	    xmlFree(tmp);

	    return;
	}
	list = list->next;
    }

    /*
     * Create a new property.
     */
    tmp = xmlEncodeEntitiesReentrant(desc->doc, BAD_CAST value);
    xmlNewChild(desc, ns, BAD_CAST property, tmp);
    xmlFree(tmp);
}

/*
 * browse the various property pertaining to this description,
 * and if found remove and destroy the subtree.
 */
void rdfRemoveProperty(rdfDescription desc, const char *property,
                       rdfNamespace ns) {
    rdfElement list = desc->xmlChildrenNode;
    rdfElement prev = NULL;

    /*
     * Search the property.
     */
    while (list != NULL) {
        if ((list->ns == ns) && (xmlStrEqual(list->name, BAD_CAST property))) {
	    /*
	     * Property was found, delete it !
	     */
	    if (prev == NULL)
	        desc->xmlChildrenNode = list->next;
	    else
	        prev->next = list->next;
	    list->next = NULL;
	    
	    xmlFreeNode(list);
	    return;
	}
	prev = list;
	list = list->next;
    }
}

/**
 ** Routines to read/write bags
 **/

/*
 * Create a new Bag.
 */
rdfBag rdfBagCreate(rdfSchema schema, rdfDescription desc,
                    const char *property, rdfNamespace ns) {
    rdfElement cur;
    rdfNamespace rdf;

    rdf = rdfGetRdfNamespace(schema);
    cur = xmlNewChild(desc, ns, BAD_CAST property, NULL);
    cur = xmlNewChild(cur, rdf, BAD_CAST "Bag", NULL);
    return(cur);
}

/*
 * Add an element to a bag.
 */
rdfElement rdfBagAddValue(rdfBag bag, const char *property,
                    rdfNamespace ns, const char *value, rdfElement elem) {
    rdfElement cur;

    if (value != NULL)
        cur = xmlNewChild(bag, ns, BAD_CAST property, BAD_CAST value);
    else {
        cur = xmlNewDocNode(bag->doc, ns, BAD_CAST property, NULL);
	if (elem != NULL)
	    xmlAddChild(cur, elem);
	xmlAddChild(bag, cur);
    }
    return(cur);
}

/**
 ** Routines to walk bags and sequences.
 **/
rdfElement rdfFirstChild(rdfElement bag) {
    rdfElement elem = bag->xmlChildrenNode;

    while ((elem != NULL) && (xmlIsBlankNode(elem)))
	    elem = elem->next;
    return(elem);
}
rdfElement rdfNextElem(rdfElement desc) {
    rdfElement elem = desc->next;

    while ((elem != NULL) && (xmlIsBlankNode(elem)))
	    elem = elem->next;
    return(elem);
}

/**
 ** Direct access to Element values.
 **/
int rdfElemGetType(rdfElement elem) {
    if (xmlStrEqual(elem->name, BAD_CAST "Bag")) return(RDF_BAG);
    if (xmlStrEqual(elem->name, BAD_CAST "Seq")) return(RDF_SEQ);
    if (xmlStrEqual(elem->name, BAD_CAST "Alt")) return(RDF_ALT);
    if (xmlStrEqual(elem->name, BAD_CAST "Description")) return(RDF_DESC);
    return(0);
}

char *rdfElemGetValue(rdfElement elem) {
    return((char *) xmlNodeListGetString(elem->doc, elem->xmlChildrenNode, 1));
}
char *rdfElemGetPropertyName(rdfElement elem) {
    return(strdup((const char *) elem->name));
}
rdfNamespace rdfElemGetNamespace(rdfElement elem) {
    return(elem->ns);
}
void rdfElemSetValue(rdfElement elem, const char *value) {
    xmlChar *tmp = xmlEncodeEntitiesReentrant(elem->doc, BAD_CAST value);

    if (tmp == NULL) return;
    xmlNodeSetContent(elem, tmp);
    xmlFree(tmp);
}

/************************************************************************
 *									*
 *				Debug					*
 *									*
 ************************************************************************/

#ifdef DEBUG_RDF
int main(void) {
    rdfSchema rdf;
    rdfNamespace rpm;
    rdfDescription desc;
    rdfBag bag;

    char *value;

    /*
     * build a fake RDF document
     */
    rdf = rdfNewSchema();
    rpm = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");
    desc = rdfAddDescription(rdf, NULL,
           "ftp://ftp.redhat.com/pub/contrib/i386/rpm2html-0.90-1.i386.rpm");
    rdfSetValue(desc, "Description", rpm, "<shade/stick & close/destroy>");
    rdfSetValue(desc, "Name", rpm, "rpm2html");
    rdfSetValue(desc, "Version", rpm, "0.90");
    rdfSetValue(desc, "Release", rpm, "1");
    bag = rdfBagCreate(rdf, desc, "Provides", rpm);
    rdfBagAddValue(bag, "Resource", rpm, "rpm2html", NULL);
    bag = rdfBagCreate(rdf, desc, "Requires", rpm);
    rdfBagAddValue(bag, "Resource", rpm, "libz.so.1", NULL);
    rdfBagAddValue(bag, "Resource", rpm, "libdb.so.2", NULL);
    rdfBagAddValue(bag, "Resource", rpm, "libc.so.6", NULL);
    rdfBagAddValue(bag, "Resource", rpm, "ld-linux.so.2", NULL);

    /*
     * Save it.
     */
    rdfWrite(rdf, "test.rdf");
    rdfDestroySchema(rdf);

    /*
     * Read it.
     */
    rdf = rdfRead("test.rdf");

    /*
     * print it.
     */
    rpm = rdfGetNamespace(rdf, "http://www.rpm.org/");
    if (rpm == NULL) fprintf(stderr, "RPM namespace not found !\n");
    desc = rdfFirstDescription(rdf);
    rdfGetValue(desc, "Name", rpm, &value, NULL);
    printf("Name : %s\n", value);
    xmlFree(value);
    rdfGetValue(desc, "Name", NULL, &value, NULL);
    printf("Name : %s\n", value);
    xmlFree(value);
    printf("\n---\n");
    xmlDocDump(stdout, rdf);

    /*
     * free it.
     */
    rdfDestroySchema(rdf);
    return(0);
}
#endif
