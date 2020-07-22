/*---------------------------------------------------------------------------*\
 *
 *  bitpit
 *
 *  Copyright (C) 2015-2019 OPTIMAD engineering Srl
 *
 *  -------------------------------------------------------------------------
 *  License
 *  This file is part of bitpit.
 *
 *  bitpit is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License v3 (LGPL)
 *  as published by the Free Software Foundation.
 *
 *  bitpit is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with bitpit. If not, see <http://www.gnu.org/licenses/>.
 *
\*---------------------------------------------------------------------------*/

#include "bitpit_common.hpp"

#include "surfunstructured.hpp"

namespace bitpit {

/*!
	\class SurfUnstructured
	\ingroup surfacepatches

	\brief The SurfUnstructured class defines an unstructured surface
	triangulation.

	SurfUnstructured defines an unstructured surface triangulation.
*/

/*!
	Creates an uninitialized patch.
*/
SurfUnstructured::SurfUnstructured()
	: SurfaceKernel(true)
{
#if BITPIT_ENABLE_MPI==1
	// This patch supports partitioning
	setPartitioningStatus(PARTITIONING_CLEAN);
#endif
}

/*!
	Creates a new patch.

	\param dimension is the dimension of the patch
*/
SurfUnstructured::SurfUnstructured(int dimension)
	: SurfaceKernel(PatchManager::AUTOMATIC_ID, dimension, true)
{
#if BITPIT_ENABLE_MPI==1
	// This patch supports partitioning
	setPartitioningStatus(PARTITIONING_CLEAN);
#endif
}

/*!
	Creates a new patch.

	\param id is the id of the patch
	\param dimension is the dimension of the patch
*/
SurfUnstructured::SurfUnstructured(int id, int dimension)
	: SurfaceKernel(id, dimension, true)
{
#if BITPIT_ENABLE_MPI==1
	// This patch supports partitioning
	setPartitioningStatus(PARTITIONING_CLEAN);
#endif
}

/*!
	Creates a new patch restoring the patch saved in the specified stream.

	\param stream is the stream to read from
*/
SurfUnstructured::SurfUnstructured(std::istream &stream)
	: SurfaceKernel(false)
{
#if BITPIT_ENABLE_MPI==1
	// This patch supports partitioning
	setPartitioningStatus(PARTITIONING_CLEAN);
#endif

	// Restore the patch
	restore(stream);
}

/*!
	Creates a clone of the pach.

	\result A clone of the pach.
*/
std::unique_ptr<PatchKernel> SurfUnstructured::clone() const
{
	return std::unique_ptr<SurfUnstructured>(new SurfUnstructured(*this));
}

/*!
 * Enables or disables expert mode.
 *
 * When expert mode is enabled, it will be possible to change the
 * patch using low level functions (e.g., it will be possible to
 * add individual cells, add vertices, delete cells, ...).
 *
 * \param expert if true, the expert mode will be enabled
 */
void SurfUnstructured::setExpert(bool expert)
{
	SurfaceKernel::setExpert(expert);
}

/*!
 *  Get the version associated to the binary dumps.
 *
 *  \result The version associated to the binary dumps.
 */
int SurfUnstructured::_getDumpVersion() const
{
	const int DUMP_VERSION = 5;

	return DUMP_VERSION;
}

/*!
 *  Write the patch to the specified stream.
 *
 *  \param stream is the stream to write to
 */
void SurfUnstructured::_dump(std::ostream &stream) const
{
	// Save the vertices
	dumpVertices(stream);

	// Save the cells
	dumpCells(stream);

	// Save the interfaces
	dumpInterfaces(stream);
}

/*!
 *  Restore the patch from the specified stream.
 *
 *  \param stream is the stream to read from
 */
void SurfUnstructured::_restore(std::istream &stream)
{
	// Restore the vertices
	restoreVertices(stream);

	// Restore the cells
	restoreCells(stream);

	// Restore the interfaces
	restoreInterfaces(stream);
}

/*!
 * Locates the cell the contains the point.
 *
 * If the point is not inside the patch, the function returns the id of the
 * null element.
 *
 * NOTE: this function is not implemented yet.
 *
 * \param[in] point is the point to be checked
 * \result Returns the linear id of the cell the contains the point. If the
 * point is not inside the patch, the function returns the id of the null
 * element.
 */
long SurfUnstructured::locatePoint(const std::array<double, 3> &point) const
{
	BITPIT_UNUSED(point);

	throw std::runtime_error ("The function 'locatePoint' is not implemented yet");

	return false;
}

//TODO: Aggiungere un metodo in SurfUnstructured per aggiungere più vertici.
/*!
 * Extract the edge network from surface mesh. If adjacencies are not built
 * edges shared by more than 1 element are counted twice. Edges are appended
 * to the content of the input SurfUnstructured
 * 
 * \param[in,out] net on output stores the network of edges
*/
void SurfUnstructured::extractEdgeNetwork(LineUnstructured &net)
{
    // ====================================================================== //
    // VARIABLES DECLARATION                                                  //
    // ====================================================================== //

    // Local variables
    bool                                        check;
    int                                         n_faces, n_adj;
    long                                        id;
    const long                                  *adjacencies;

    // Counters
    int                                         i, j;
    VertexIterator                              v_, ve_ = vertexEnd();
    CellIterator                                c_, ce_ = cellEnd();

    // ====================================================================== //
    // INITIALIZE DATA STRUCTURE                                              //
    // ====================================================================== //
    net.reserveCells(net.getCellCount() + countFaces());
    net.reserveVertices(net.getVertexCount() + getVertexCount());

    // ====================================================================== //
    // ADD VERTEX TO net                                                      //
    // ====================================================================== //
    for (v_ = vertexBegin(); v_ != ve_; ++v_) {
        net.addVertex(v_->getCoords(), v_->getId());
    } //next v_

    // ====================================================================== //
    // ADD EDGES                                                              //
    // ====================================================================== //
    for (c_ = cellBegin(); c_ != ce_; ++c_) {
        id = c_->getId();
        n_faces = c_->getFaceCount();
        ConstProxyVector<long> cellVertexIds = c_->getVertexIds();
        for (i = 0; i < n_faces; ++i) {
            check = true;
            n_adj = c_->getAdjacencyCount(i);
            adjacencies = c_->getAdjacencies(i);
            for (j = 0; j < n_adj; ++j) {
                check = check && (id > adjacencies[j]);
            } //next j
            if (check) {
                // Get edge type
                ElementType edgeType = c_->getFaceType(i);

                // Get edge connect
                ConstProxyVector<long> faceConnect = c_->getFaceConnect(i);
                int faceConnectSize = faceConnect.size();

                std::unique_ptr<long[]> edgeConnect = std::unique_ptr<long[]>(new long[faceConnectSize]);
                for (int k = 0; k < faceConnectSize; ++k) {
                    edgeConnect[k] = faceConnect[k];
                }

                // Add edge
                net.addCell(edgeType, std::move(edgeConnect));
            }
        } //next i
    } //next c_

    return;
}

//TODO: normals??
//TODO: error flag on output
//TODO: import a specified solid (ascii format only)
/*!
 * Import surface tasselation from S.T.L. file. STL facet are added at to the
 * present mesh, i.e. current mesh content is not discarded. However no checks
 * are performed to ensure that no duplicated vertices or cells are created.
 *
 * If the input file is a multi-solid ASCII file, all solids will be loaded
 * and a different PID will be assigned to the PID of the different solids.
 * 
 * \param[in] filename name of stl file
 * \param[in] PIDOffset is the offset for the PID numbering
 * \param[in] PIDSquash controls if the PID of the cells will be read from
 * the file or if the same PID will be assigned to all cells
 * 
 * \result on output returns an error flag for I/O error
*/
int SurfUnstructured::importSTL(const std::string &filename,
                                int PIDOffset, bool PIDSquash)
{
    return importSTL(filename, STLReader::FormatUnknown, PIDOffset, PIDSquash);
}

/*!
 * Import surface tasselation from S.T.L. file. STL facet are added at to the
 * present mesh, i.e. current mesh content is not discarded. However no checks
 * are performed to ensure that no duplicated vertices or cells are created.
 *
 * If the input file is a multi-solid ASCII file, all solids will be loaded
 * and a different PID will be assigned to the PID of the different solids.
 *
 * \param[in] filename name of stl file
 * \param[in] isBinary flag for binary (true), of ASCII (false) stl file
 * \param[in] PIDOffset is the offset for the PID numbering
 * \param[in] PIDSquash controls if the PID of the cells will be read from
 * the file or if the same PID will be assigned to all cells
 * \param[in,out] PIDNames are the names of the PIDs, on output the names
 * of the newly imported PIDs will be added to the list
 *
 * \result on output returns an error flag for I/O error
*/
int SurfUnstructured::importSTL(const std::string &filename, bool isBinary,
                                int PIDOffset, bool PIDSquash,
                                std::unordered_map<int, std::string> *PIDNames)
{
    STLReader::Format format;
    if (isBinary) {
        format = STLReader::FormatBinary;
    } else {
        format = STLReader::FormatASCII;
    }

    return importSTL(filename, format, PIDOffset, PIDSquash, PIDNames);
}

/*!
 * Import surface tasselation from S.T.L. file. STL facet are added at to the
 * present mesh, i.e. current mesh content is not discarded. However no checks
 * are performed to ensure that no duplicated vertices or cells are created.
 *
 * If the input file is a multi-solid ASCII file, all solids will be loaded
 * and a different PID will be assigned to the PID of the different solids.
 *
 * \param[in] filename name of stl file
 * \param[in] format is the format of stl file
 * \param[in] PIDOffset is the offset for the PID numbering
 * \param[in] PIDSquash controls if the PID of the cells will be read from
 * the file (false) or if the same PID will be assigned to all cells (true).
 * \param[in,out] PIDNames are the names of the PIDs, on output the names
 * of the newly imported PIDs will be added to the list
 *
 * \result on output returns an error flag for I/O error
*/
int SurfUnstructured::importSTL(const std::string &filename, STLReader::Format format,
                                int PIDOffset, bool PIDSquash,
                                std::unordered_map<int, std::string> *PIDNames)
{
    int readerError;

    // Initialize reader
    STLReader reader(filename, format);
    if (format == STLReader::FormatUnknown) {
        format = reader.getFormat();
    }

    // Begin reding the STL file
    readerError = reader.readBegin();
    if (readerError != 0) {
        return readerError;
    }

    // Read all the solids in the STL file
    int pid = PIDOffset;
    if (!PIDSquash) {
        --pid;
    }

    ElementType facetType = ElementType::TRIANGLE;
    int nFacetVertices = ReferenceElementInfo::getInfo(ElementType::TRIANGLE).nVertices;

    while (true) {
        // Read header
        std::size_t nFacets;
        std::string name;
        readerError = reader.readHeader(&name, &nFacets);
        if (readerError != 0) {
            if (readerError == -2) {
                break;
            } else {
                return readerError;
            }
        }

        // Generate patch cells from STL facets
        reserveVertices(getVertexCount() + nFacetVertices * nFacets);
        reserveCells(getCellCount() + nFacets);

        for (std::size_t n = 0; n < nFacets; ++n) {
            // Read facet data
            std::array<double, 3> coords_0;
            std::array<double, 3> coords_1;
            std::array<double, 3> coords_2;
            std::array<double, 3> normal;

            readerError = reader.readFacet(&coords_0, &coords_1, &coords_2, &normal);
            if (readerError != 0) {
                return readerError;
            }

            // Add vertices
            VertexIterator vertexItr_0 = addVertex(coords_0);
            VertexIterator vertexItr_1 = addVertex(coords_1);
            VertexIterator vertexItr_2 = addVertex(coords_2);

            // Add cell
            std::unique_ptr<long[]> connectStorage = std::unique_ptr<long[]>(new long[nFacetVertices]);
            connectStorage[0] = vertexItr_0.getId();
            connectStorage[1] = vertexItr_1.getId();
            connectStorage[2] = vertexItr_2.getId();

            CellIterator cellIterator = addCell(facetType, std::move(connectStorage));
            cellIterator->setPID(pid);
        }

        // Read footer
        readerError = reader.readFooter(name);
        if (readerError != 0) {
            return readerError;
        }

        // Assign PID name
        if (!PIDSquash) {
            ++pid;
            if (PIDNames && !name.empty()) {
                PIDNames->insert({{pid, name}});
            }
        }

        // Multi-body STL files are supported only in ASCII mode
        if (format == STLReader::FormatBinary) {
            break;
        }
    }

    // End reading
    readerError = reader.readEnd();
    if (readerError != 0) {
        return readerError;
    }

    return 0;
}

/*!
 * Export surface tasselation in a STL format. No check is perfomed on element type
 * therefore tasselation containing vertex, line or quad elements will produce
 * ill-formed stl triangulation.
 *
 * \param[in] filename name of the stl file
 * \param[in] isBinary flag for binary (true) or ASCII (false) file
 * \param[in] exportInternalsOnly flag for exporting only internal cells (true), or
 * internal and ghost cells (false).
 *
 * \result on output returns an error flag for I/O error.
 */
int SurfUnstructured::exportSTL(const std::string &filename, bool isBinary, bool exportInternalsOnly)
{
    return exportSTLSingle(filename, isBinary, exportInternalsOnly);
}

/*!
 * Export surface tasselation in a STL format. No check is perfomed on element type
 * therefore tasselation containing vertex, line or quad elements will produce
 * ill-formed stl triangulation. Overloading supporting the the ascii multi-solid mode export.
 *
 * \param[in] filename name of the stl file
 * \param[in] isBinary flag for binary (true) or ASCII (false) file
 * \param[in] isMulti flag to write in ASCII multi-solid mode (true) or not (false).
 * If true, isBinary flag will be ignored.
 * \param[in] exportInternalsOnly flag for exporting only internal cells (true), or
 * internal+ghost cells (false).
 * \param[in,out] PIDNames are the names of the PIDs, if a PIDs has no name
 * its number will be used
 * \result on output returns an error flag for I/O error.
 */
int SurfUnstructured::exportSTL(const std::string &filename, bool isBinary,
                                bool isMulti, bool exportInternalsOnly,
                                std::unordered_map<int, std::string> *PIDNames)
{
    int flag = 0;
    if (isMulti) {
        flag = exportSTLMulti(filename, exportInternalsOnly, PIDNames);
    } else {
        flag = exportSTLSingle(filename, isBinary, exportInternalsOnly);
    }

    return flag;
}

//TODO: normals??
//TODO: error flag on output
//TODO: conversion of quad into tria
/*!
 * Export surface tasselation in a STL format. No check is perfomed on element type
 * therefore tasselation containing vertex, line or quad elements will produce
 * ill-formed stl triangulation.
 * 
 * \param[in] filename name of the stl file
 * \param[in] isBinary flag for binary (true) or ASCII (false) file
 * \param[in] exportInternalsOnly flag for exporting only internal cells (true),
 * or internal+ghost cells (false).
 * 
 * \result on output returns an error flag for I/O error.
*/
int SurfUnstructured::exportSTLSingle(const std::string &filename, bool isBinary, bool exportInternalsOnly)
{
    int writerError;

    // Initialize writer
    STLReader::Format format;
    if (isBinary) {
        format = STLReader::FormatBinary;
    } else {
        format = STLReader::FormatASCII;
    }

    STLWriter writer(filename, format);

    // Solid name
    //
    // An ampty solid name will be used.
    const std::string name = "";

    // Begin writing
    writerError = writer.writeBegin(STLWriter::WriteOverwrite);
    if (writerError != 0) {
        writer.writeEnd();

        return writerError;
    }

    // Write header
    std::size_t nFacets = getInternalCount();
#if BITPIT_ENABLE_MPI==1
    if (!exportInternalsOnly) {
        nFacets += getGhostCount();
    }
#endif

    writerError = writer.writeHeader(name, nFacets);
    if (writerError != 0) {
        writer.writeEnd();

        return writerError;
    }

    // Write facet data
    CellConstIterator cellBegin;
    CellConstIterator cellEnd;
    if (exportInternalsOnly) {
        cellBegin = internalConstBegin();
        cellEnd   = internalConstEnd();
    } else {
        cellBegin = cellConstBegin();
        cellEnd   = cellConstEnd();
    }

    for (CellConstIterator cellItr = cellBegin; cellItr != cellEnd; ++cellItr) {
        // Get cell
        const Cell &cell = *cellItr;

        // Get vertex coordinates
        ConstProxyVector<long> cellVertexIds = cell.getVertexIds();
        assert(cellVertexIds.size() == 3);

        const std::array<double, 3> &coords_0 = getVertex(cellVertexIds[0]).getCoords();
        const std::array<double, 3> &coords_1 = getVertex(cellVertexIds[1]).getCoords();
        const std::array<double, 3> &coords_2 = getVertex(cellVertexIds[2]).getCoords();

        // Evaluate normal
        const std::array<double, 3> normal = evalFacetNormal(cell.getId());

        // Write data
        writerError = writer.writeFacet(coords_0, coords_1, coords_2, normal);
        if (writerError != 0) {
            writer.writeEnd();

            return writerError;
        }
    }

    // Write footer
    writerError = writer.writeFooter(name);
    if (writerError != 0) {
        writer.writeEnd();

        return writerError;
    }

    // End writing
    writerError = writer.writeEnd();
    if (writerError != 0) {
        return writerError;
    }

    return 0;
}

/*!
 * Export surface tasselation in a STL Multi Solid format, in ascii mode only. Binary is not supported for STL Multisolid.
 * No check is perfomed on element type therefore tasselation containing vertex, line or quad elements will produce
 * ill-formed stl triangulation. If available, ghost cells will be written in a stand-alone solid.
 *
 * \param[in] filename name of the stl file
 * \param[in] exportInternalsOnly OPTIONAL flag for exporting only internal cells (true),
 * or internal+ghost cells (false). Default is true.
  * \param[in,out] PIDNames are the names of the PIDs, if a PIDs has no name
  * its number will be used
 * \result on output returns an error flag for I/O error 0-done, >0 errors.
 */
int SurfUnstructured::exportSTLMulti(const std::string &filename, bool exportInternalsOnly,
                                     std::unordered_map<int, std::string> *PIDNames)
{
#if not BITPIT_ENABLE_MPI==1
    BITPIT_UNUSED(exportInternalsOnly);
#endif

    int writerError;

    // Initialize writer
    STLWriter writer(filename, STLReader::FormatASCII);

    // Begin writing
    writerError = writer.writeBegin(STLWriter::WriteOverwrite);
    if (writerError != 0) {
        return writerError;
    }

    // Export the internal cells
    for (int pid : getInternalPIDs()) {
        // Cells associated to the PID
        std::vector<long> cells = getInternalsByPID(pid);

        // Write header
        std::string name;
        if (PIDNames && PIDNames->count(pid) > 0) {
            name = PIDNames->at(pid);
        } else {
            name = std::to_string(pid);
        }

        std::size_t nFacets = cells.size();

        writerError = writer.writeHeader(name, nFacets);
        if (writerError != 0) {
            writer.writeEnd();

            return writerError;
        }

        // Write facet data
        for (long cellId : cells) {
            // Get cell
            const Cell &cell = getCell(cellId);

            // Get vertex coordinates
            ConstProxyVector<long> cellVertexIds = cell.getVertexIds();
            assert(cellVertexIds.size() == 3);

            const std::array<double, 3> &coords_0 = getVertex(cellVertexIds[0]).getCoords();
            const std::array<double, 3> &coords_1 = getVertex(cellVertexIds[1]).getCoords();
            const std::array<double, 3> &coords_2 = getVertex(cellVertexIds[2]).getCoords();

            // Evaluate normal
            const std::array<double, 3> normal = evalFacetNormal(cellId);

            // Write data
            writerError = writer.writeFacet(coords_0, coords_1, coords_2, normal);
            if (writerError != 0) {
                writer.writeEnd();

                return writerError;
            }
        }

        // Write footer
        writerError = writer.writeFooter(name);
        if (writerError != 0) {
            writer.writeEnd();

            return writerError;
        }
    }

#if BITPIT_ENABLE_MPI==1
    // Export ghost cells
    long nGhosts = getGhostCount();
    if (!exportInternalsOnly && nGhosts > 0) {
        // Write header
        std::string name = "ghosts";

        std::size_t nFacets = nGhosts;

        writerError = writer.writeHeader(name, nFacets);
        if (writerError != 0) {
            writer.writeEnd();

            return writerError;
        }

        // Write facet data
        CellConstIterator cellBegin = internalConstBegin();
        CellConstIterator cellEnd   = internalConstEnd();
        for (CellConstIterator cellItr = cellBegin; cellItr != cellEnd; ++cellItr) {
            // Get cell
            const Cell &cell = *cellItr;

            // Get vertex coordinates
            ConstProxyVector<long> cellVertexIds = cell.getVertexIds();
            assert(cellVertexIds.size() == 3);

            const std::array<double, 3> &coords_0 = getVertex(cellVertexIds[0]).getCoords();
            const std::array<double, 3> &coords_1 = getVertex(cellVertexIds[1]).getCoords();
            const std::array<double, 3> &coords_2 = getVertex(cellVertexIds[2]).getCoords();

            // Evaluate normal
            const std::array<double, 3> normal = evalFacetNormal(cell.getId());

            // Write data
            writerError = writer.writeFacet(coords_0, coords_1, coords_2, normal);
            if (writerError != 0) {
                writer.writeEnd();

                return writerError;
            }
        }

        // Write footer
        writerError = writer.writeFooter(name);
        if (writerError != 0) {
            writer.writeEnd();

            return writerError;
        }
    }
#endif

    // End writing
    writerError = writer.writeEnd();
    if (writerError != 0) {
        return writerError;
    }

    return 0;
}

/*!
 * Import surface tasselation from DGF file.
 * 
 * \param[in] filename name of dgf file
 * \param[in] PIDOffset is the offset for the PID numbering
 * \param[in] PIDSquash controls if the PID of the cells will be read from
 * the file or if the same PID will be assigned to all cells
 * 
 * \result on output returns an error flag for I/O error.
*/
int SurfUnstructured::importDGF(const std::string &filename, int PIDOffset, bool PIDSquash)
{
    // ====================================================================== //
    // VARIABLES DECLARATION                                                  //
    // ====================================================================== //

    // Local variables
    DGFObj                                                      dgf_in(filename);
    int                                                         nV = 0, nS = 0;
    long                                                        vcount, idx;
    std::vector<std::array<double, 3>>                          vertex_list;
    std::vector<std::vector<int>>                               simplex_list;
    std::vector<int>                                            simplex_PID;
    std::vector<long>                                           vertex_map;
    std::vector<long>                                           connect;

    // Counters
    std::vector<std::array<double, 3>>::const_iterator          v_, ve_;
    std::vector<std::vector<int>>::iterator                     c_, ce_;
    std::vector<int>::iterator                                  i_, ie_;
    std::vector<long>::iterator                                 j_, je_;

    // ====================================================================== //
    // IMPORT DATA                                                            //
    // ====================================================================== //

    // Read vertices and cells from DGF file
    dgf_in.load(nV, nS, vertex_list, simplex_list, simplex_PID);

    // Add vertices
    ve_ = vertex_list.cend();
    vcount = 0;
    vertex_map.resize(nV);
    for (v_ = vertex_list.cbegin(); v_ != ve_; ++v_) {
        idx = addVertex(*v_)->getId();
        vertex_map[vcount] = idx;
        ++vcount;
    } //next v_

    // Update connectivity infos
    ce_ = simplex_list.end();
    for (c_ = simplex_list.begin(); c_ != ce_; ++c_) {
        ie_ = c_->end();
        for (i_ = c_->begin(); i_ != ie_; ++i_) {
            *i_ = vertex_map[*i_];
        } //next i_
    } //next c_

    // Add cells
    int k;
    for (c_ = simplex_list.begin(), k = 0; c_ != ce_; ++c_, ++k) {
        // Create cell
        i_ = c_->begin();
        connect.resize(c_->size(), Vertex::NULL_ID);
        je_ = connect.end();
        for (j_ = connect.begin(); j_ != je_; ++j_) {
            *j_ = *i_;
            ++i_;
        } //next j_
        CellIterator cellIterator = addCell(getDGFFacetType(c_->size()), connect);

        // Set cell PID
        int cellPID = PIDOffset;
        if (!PIDSquash) {
            cellPID += simplex_PID[k];
        }

        cellIterator->setPID(cellPID);
    } //next c_

    return 0;
}

/*!
 * Export surface tasselation to DGF file
 * 
 * \param[in] filename name of dgf file
 * 
 * \result on output returns an error flag for I/O error
*/
int SurfUnstructured::exportDGF(const std::string &filename)
{
    // ====================================================================== //
    // VARIABLES DECLARATION                                                  //
    // ====================================================================== //

    // Local variables
    DGFObj                                                      dgf_in(filename);
    int                                                         nV = getVertexCount(), nS = getCellCount();
    int                                                         v, nv;
    long                                                        vcount, ccount, idx;
    std::vector<std::array<double, 3>>                          vertex_list(nV);
    std::vector<std::vector<int>>                               simplex_list(nS);
    std::unordered_map<long, long>                              vertex_map;

    // Counters
    VertexIterator                                              v_, ve_;
    CellIterator                                                c_, ce_;

    // ====================================================================== //
    // EXPORT DATA                                                            //
    // ====================================================================== //

    // Create vertex list
    ve_ = vertexEnd();
    vcount = 0;
    for (v_ = vertexBegin(); v_ != ve_; ++v_) {
        idx = v_->getId();
        vertex_list[vcount] = v_->getCoords();
        vertex_map[idx] = vcount;
        ++vcount;
    } //next v_

    // Add cells
    ce_ = cellEnd();
    ccount = 0;
    for (c_ = cellBegin(); c_ != ce_; ++c_) {
        ConstProxyVector<long> cellVertexIds = c_->getVertexIds();
        nv = cellVertexIds.size();
        simplex_list[ccount].resize(nv);
        for (v = 0; v < nv; ++v) {
            simplex_list[ccount][v] = vertex_map[cellVertexIds[v]];
        } //next v
        ++ccount;
    } //next c_

    // Read vertices and cells from DGF file
    dgf_in.save(nV, nS, vertex_list, simplex_list);

    return 0;
}

/*!
* Get the element type of a facet with the specified number of vertices.
*
* \param[in] nFacetVertices is the number of the vertices of the facet
*
* \result The element type of a facet with the specified number of vertices.
*/
ElementType SurfUnstructured::getDGFFacetType(int nFacetVertices)
{
    switch(nFacetVertices) {

    case 1:
        return ElementType::VERTEX;

    case 2:
        return ElementType::LINE;

    case 3:
        return ElementType::TRIANGLE;

    case 4:
        return ElementType::QUAD;

    default:
        return ElementType::UNDEFINED;

    }
}

}
