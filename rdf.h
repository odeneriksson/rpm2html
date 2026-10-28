/*
 * rdf.h : interfaces for the RDF encoding/decoding of RPM information.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rdf.h,v 1.8 2010/10/05 14:36:54 hany Exp $
 */

#ifndef __INCLUDE_RDF_H__
#define __INCLUDE_RDF_H__

extern rpmDataPtr rpmOpenRdfFile(char *file);
extern rpmDataPtr rpmOpenRdf(char *nameRpm, rpmDirPtr dir, rpmSubdirPtr tree);
extern void dumpRpmRdf(rpmDataPtr rpm, rpmSubdirPtr tree);
extern void dumpAllResRdf(void);
extern void dumpResRdf(rpmRessPtr res);
extern void dumpAproposRdf(void);
extern void dumpDistListRdf(void);

#endif /* __INCLUDE_RDF_H__ */
