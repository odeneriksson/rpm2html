/*
 * rdf_api.h : interface definitions to read and write RDF schemas
 *
 * See Copyright for the status of this software.
 *
 * $Id: rdf_api.h,v 1.12 2002/11/16 11:34:15 veillard Exp $
 */

#ifndef __RDF_API_H__
#define __RDF_API_H__
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "rdf.h"

/*
 * An RDF schema is basically an XML document.
 */
typedef xmlDocPtr rdfSchema;

/*
 * An RDF namespace is an XML namespace/Ns
 */
typedef xmlNsPtr rdfNamespace;

/*
 * Any RDF element is an XML element.
 */
typedef xmlNodePtr rdfElement;

/*
 * An RDF description is an RDF element and usually
 * a direct child of the root for simple descriptions.
 */
typedef rdfElement rdfDescription;

/*
 * The value of an element can be either:
 *   - A piece of Text : char *			RDF_LEAF
 *   - A collection tag :
 *       - Bag,					RDF_BAG
 *       - Seq					RDF_SEQ
 *       - Alt					RDF_ALT
 *   - An rdfDescription			RDF_DESC
 */

#define RDF_LEAF	1
#define RDF_BAG		2
#define RDF_SEQ		3
#define RDF_ALT		4
#define RDF_DESC	5

/*
 * An RDF Bag is and RDF element with possibly multiple child
 */
typedef rdfElement rdfBag;

/*
 * An RDF Seq is and RDF element an enumeration of childs
 */
typedef rdfElement rdfSeq;

/*
 * Basic routines reading/writing an RDF file.
 */
rdfSchema rdfRead(const char *filename);
void rdfWrite(rdfSchema rdf, const char *filename);
void rdfWriteMemory(rdfSchema rdf, char **buffer, int *size);

/*
 * An RDF schema is a collection of RDF descriptions.
 */
rdfSchema rdfNewSchema(void);
void rdfDestroySchema(rdfSchema rdf);
rdfDescription rdfFirstDescription(rdfSchema schema);
rdfDescription rdfNextDescription(rdfDescription desc);
rdfDescription rdfAddDescription(rdfSchema schema, const char *id,
                                 const char *about);
char *rdfGetDescriptionId(rdfSchema schema, rdfDescription desc);
char *rdfGetDescriptionAbout(rdfSchema schema, rdfDescription desc);
char *rdfGetElementResource(rdfSchema schema, rdfElement elem);
#define rdfGetDescriptionHref(s, d) rdfGetDescriptionAbout((s), (d))
void rdfSetElementResource(rdfSchema schema, rdfElement elem, const char *URI);

/*
 * Namespace handling.
 */
rdfNamespace rdfNewNamespace(rdfSchema rdf, const char *url, const char *ns);
rdfNamespace rdfGetNamespace(rdfSchema rdf, const char *href);
rdfNamespace rdfGetRdfNamespace(rdfSchema rdf);

/*
 * Routines to read/write values, which can be either final ones or
 * subtree.
 */
int rdfGetValue(rdfDescription desc, const char *property,
                rdfNamespace ns, char **value, rdfElement *elem);
void rdfSetValue(rdfDescription desc, const char *property,
                rdfNamespace ns, const char *value);
void rdfSetTree(rdfDescription desc, const char *property,
                rdfNamespace ns, rdfElement elem);
void rdfRemoveProperty(rdfDescription desc, const char *property,
                       rdfNamespace ns);

/*
 * Routines to read/write bags
 */
rdfBag rdfBagCreate(rdfSchema schema, rdfDescription desc,
                    const char *property, rdfNamespace ns);
rdfElement rdfBagAddValue(rdfBag bag, const char *property, 
                    rdfNamespace ns, const char *value, rdfElement elem);

/*
 * Routines to walk bags and sequences.
 */
rdfElement rdfFirstChild(rdfElement bag);
rdfElement rdfNextElem(rdfElement desc);

/*
 * Direct access to Element values.
 */
int rdfElemGetType(rdfElement elem);
char *rdfElemGetValue(rdfElement elem);
char *rdfElemGetPropertyName(rdfElement elem);
rdfNamespace rdfElemGetNamespace(rdfElement elem);
void rdfElemSetValue(rdfElement elem, const char *value);

#endif /* __RDF_API_H__ */
