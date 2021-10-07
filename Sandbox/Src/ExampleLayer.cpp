﻿#include "ExampleLayer.h"
#include <iomanip>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <fstream>
#include <mutex>
#include <atomic>
using namespace GLCore;
using namespace GLCore::Utils;

// Static data begin
constexpr uint32_t VertexBatchSize = 128*128*4;
const char *ViewProjectionIdentifierInShader = "u_ViewProjectionMat4";
const char *ModelMatrixIdentifierInShader = "u_ModelMat4";
// Static data end

bool MeanCurvatureCalculate (const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &color);
bool MeanCurvature2Calculate (const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &color);

bool ExampleLayer::LoadModel (std::string filePath){
	std::vector<std::pair<glm::vec3, glm::vec3>> meshVertices;
	std::vector<uint32_t> meshIndices;
	bool meshloaded = Helper::ASSET_LOADER::LoadOBJ_meshOnly(filePath.c_str (), meshVertices, meshIndices);
	if (meshloaded) {
		{ // load to memory
			std::vector<std::pair<glm::vec3, glm::vec3>> posn_and_normal;
			std::vector<GLuint> indices;
			posn_and_normal = std::move(meshVertices);
			indices = std::move(meshIndices);
			m_MeshColorData.clear ();
			// transfer mesh
			m_StaticMeshData = std::move (posn_and_normal);
			m_MeshColorData.resize (m_StaticMeshData.size ());
			for (auto &ele : m_MeshColorData)
				ele = glm::vec3 (0.7, 0.3, 0.05);
			m_MeshIndicesData = std::move (indices);
		}

		// Upload Mesh
		if(m_MeshVA)
			glDeleteVertexArrays (1, &m_MeshVA);
		if(m_MeshSVB)
			glDeleteBuffers (1, &m_MeshSVB);
		if(m_MeshCVB)
			glDeleteBuffers (1, &m_MeshCVB);
		if(m_MeshIB)
			glDeleteBuffers (1, &m_MeshIB);

		glGenVertexArrays (1, &m_MeshVA);
		glBindVertexArray (m_MeshVA);

		glGenBuffers (1, &m_MeshSVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_MeshSVB);
		{
			size_t size = m_StaticMeshData.size ()*sizeof (glm::vec3[2]);
			glBufferData (GL_ARRAY_BUFFER, size, m_StaticMeshData.data (), GL_STATIC_DRAW);
		}
		
		glGenBuffers (1, &m_MeshCVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
		glBufferData (GL_ARRAY_BUFFER, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data(), GL_DYNAMIC_DRAW);
		
		{
			glEnableVertexAttribArray (0); // position
			glEnableVertexAttribArray (1); // normal
			glEnableVertexAttribArray (2); // color

			// glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
			uint32_t stride = sizeof (float) * 3 * 2;
			uint32_t offset1 = 0;
			uint32_t offset2 = sizeof (float) * 3;
			glBindBuffer (GL_ARRAY_BUFFER, m_MeshSVB);
			glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offset1));
			glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offset2));

			glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
			glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, sizeof (float) * 3, 0);
		}
		glGenBuffers (1, &m_MeshIB);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_MeshIB);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, m_MeshIndicesData.size ()*sizeof (GLuint), &m_MeshIndicesData[0], GL_STATIC_DRAW);
	}
	return meshloaded;
}
void ExampleLayer::OnAttach()
{
	EnableGLDebugging();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ReloadSquareShader ();

	if (m_LoadedMeshPath.empty ()) {
		std::string filepath = "./assets/Base Mesh sculpt 2.obj";
		m_LoadedMeshPath = filepath;
		if (!LoadModel (std::move (filepath))) {
			LOG_ERROR ("Cannot Load Mesh");
			m_LoadedMeshPath = "";
		}
	}
	m_Camera.ReCalculateTransform ();
	m_Camera.ReCalculateProjection (1);
}
void ExampleLayer::OnDetach()
{
	if (m_MeshVA)
		glDeleteVertexArrays (1, &m_MeshVA);
	if (m_MeshSVB)
		glDeleteBuffers (1, &m_MeshSVB);
	if (m_MeshCVB)
		glDeleteBuffers (1, &m_MeshCVB);
	if (m_MeshIB)
		glDeleteBuffers (1, &m_MeshIB);

	DeleteSquareShader ();
}
void ExampleLayer::OnUpdate(Timestep ts)
{
	glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram (m_SquareShaderProgID); // You can find shader inside base.cpp as a static c_str
	glm::mat4 viewProjMat = m_Camera.GetProjection ()*m_Camera.GetView ();
	glUniformMatrix4fv (m_Uniform.Mat4_ViewProjection, 1, GL_FALSE, glm::value_ptr (viewProjMat));
	static glm::mat4 modelMatrix = glm::scale (glm::mat4 (1.0f), glm::vec3 (3));
	glUniformMatrix4fv (m_Uniform.Mat4_ModelMatrix, 1, GL_FALSE, glm::value_ptr (modelMatrix));

	glBindVertexArray (m_MeshVA);
	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);
	glEnableVertexAttribArray (2);
	glDrawElements (GL_TRIANGLES, m_MeshIndicesData.size (), GL_UNSIGNED_INT, nullptr);
}
std::array<int, 10> just_an_arr = my_std::make_array<10> (23);
void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin(ImGuiLayer::UniqueName("Controls"));

	if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content"))) {
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
			
			if(ImGui::DragFloat3 ("Cam Position", &m_Camera.Position[0]) ||
			ImGui::DragFloat3 ("Cam Rotation", &m_Camera.Rotation[0]))
				m_Camera.ReCalculateTransform ();
			if(ImGui::DragFloat ("Near plane", &m_Camera.Near, 1.0, 0.01f) ||
			ImGui::DragFloat ("Far  plane", &m_Camera.Far, 1.0, m_Camera.Near, 10000))
				m_Camera.ReCalculateProjection (1);
			if(ImGui::Button ("CamUpdateTransform"))
				m_Camera.ReCalculateTransform ();
			ImGui::SameLine ();
			if(ImGui::Button ("CamUpdateProjection"))
				m_Camera.ReCalculateProjection (1);
			ImGui::Text ("Note:\n For some reason parsed models 1st mesh is the only one being loaded, this is 1st for me so probably a tiny mistake on my side.\nI'll Address the issue on a later date. :)");
			ImGui::EndTabItem ();

			if (ImGui::Button ("Calculate mean curvature"))
				if (MeanCurvatureCalculate (m_StaticMeshData, m_MeshIndicesData, m_MeshColorData) && m_MeshCVB) {
					glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
					glBufferSubData (GL_ARRAY_BUFFER, 0, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data ());
				}
			ImGui::SameLine ();
			if (ImGui::Button ("Calculate mean curvature 2"))
				if (MeanCurvature2Calculate (m_StaticMeshData, m_MeshIndicesData, m_MeshColorData) && m_MeshCVB) {
					glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
					glBufferSubData (GL_ARRAY_BUFFER, 0, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data ());
				}
			if (ImGui::Button ("Load Another Model")) {
				std::string filePath = GLCore::Utils::FileDialogs::OpenFile ("Model\0*.obj\0");
				if (!filePath.empty ()) {
					m_LoadedMeshPath = filePath;
					if (!LoadModel (std::move (filePath))) {
						LOG_ERROR ("Cannot Load Mesh");
						m_LoadedMeshPath = "";
					}
				}
			}
		}
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Square Shader Source"))) {

			OnImGuiSqureShaderSource ();

			ImGui::EndTabItem ();
		}
		ImGui::EndTabBar ();
	}

	ImGui::End();
}

void ExampleLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);

}
void ExampleLayer::ImGuiMenuOptions ()
{
	//
}
void ExampleLayer::OnSquareShaderReload ()
{
	m_Uniform.Mat4_ViewProjection = glGetUniformLocation (m_SquareShaderProgID, ViewProjectionIdentifierInShader);
	m_Uniform.Mat4_ModelMatrix    = glGetUniformLocation (m_SquareShaderProgID, ModelMatrixIdentifierInShader);
}


/////////////////
// Camera

const glm::mat4 ExampleLayer::Camera::GetTransform ()
{
	glm::mat4 matrix = m_RotationMatrix;
	for (uint16_t i = 0; i < 3; i++) {
		matrix[3][i] = Position[i];
	}
	return matrix;
}
const glm::mat4 &ExampleLayer::Camera::GetView ()
{
	glm::mat4 matrix = glm::transpose (m_RotationMatrix); // orthogonal
	for (uint16_t i = 0; i < 3; i++) {
		matrix[3][i] = -Position[i]; // inverted translation
	}
	return matrix;
}
const glm::mat4 &ExampleLayer::Camera::GetProjection ()
{
	return m_Projection;
}
void ExampleLayer::Camera::ReCalculateProjection (float aspectRatio)
{
	m_Projection = glm::perspective (glm::radians (FOV_y), aspectRatio, Near, Far);
}
void ExampleLayer::Camera::ReCalculateTransform ()
{
	m_RotationMatrix =
		glm::rotate (
			glm::rotate (
				glm::rotate (
					glm::mat4 (1.0f)
				,glm::radians (Rotation.y), { 0,1,0 })
			,glm::radians (Rotation.x), { 1,0,0 })
		,glm::radians (Rotation.z), { 0,0,1 });
}
void ExampleLayer::Camera::LookAt (glm::vec3 positionInSpace)
{
	glm::vec3 dirn (glm::normalize (positionInSpace - Position));
	glm::vec2 angl = glm::radians (PitchYawFromDirn (dirn *= -1.0f));//Inverting The Model Matrix Flipped the dirn of forward, so flipping the dirn to the trouble of flipping pitch yaw
	glm::vec3 eular (angl.x, angl.y, 0);
	Rotation = eular;
}
glm::vec2 ExampleLayer::Camera::PitchYawFromDirn (glm::vec3 dirn)
{
	float x = dirn.x;
	float y = dirn.y;
	float z = dirn.z;
	float xz = sqrt (x * x + z * z);//length of projection in xz plane
	float xyz = 1.0f;//i.e sqrt(y*y + xz*xz);//length of dirn

	float yaw = glm::degrees (asin (x / xz));
	float pitch = glm::degrees (asin (y / xyz));
	//front is  0, 0, 1;
	if (dirn.z < 0)
		yaw = 180 - yaw;

	//negating pitch --\/ as pitch is clockwise rotation while eular rotation is counter-Clockwise
	return dirn;
}


bool MeanCurvatureCalculate (const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint>&indices, std::vector<glm::vec3> &curvature_diffuse_color)
{
	float max_curvature = -std::numeric_limits<float>::max();
	float min_curvature =  std::numeric_limits<float>::max();
	std::vector<float> curvatures;
	curvatures.reserve (posn_and_normals.size ());
	const char *fn = "curvatureResults.txt";
#if MODE_DEBUG
	std::ofstream ofs (fn);
	if (ofs.bad ()) {
		std::cout << "File creation error - " << fn << '\n';
		__debugbreak ();
	}
	std::mutex mutex_ofs;
#endif
	std::mutex mutex_curvature_push,mutex_cout;
	std::atomic<size_t> vertices_processed = 0;
	

	auto mean_curvature_func = [&](uint32_t start, uint32_t stride) {

	#if MODE_DEBUG
		std::ostringstream out_stream;
		out_stream << std::fixed << std::setprecision (8);
	#endif
	
		for (size_t curr_indice = start; curr_indice < posn_and_normals.size (); curr_indice += stride) // repeat for every vertex
		{
			mutex_cout.lock ();
			std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size ();
			mutex_cout.unlock ();
			vertices_processed++;

			float curvatureAroundVertex = 0.0f;
			{
				std::vector<size_t> grps; // or triangular primitives or face

				std::vector<uint32_t> indices_of_neighbouring_vertives; // bucket of neighboring vertices

				for (size_t i = 0; i < indices.size (); i++) { // Scan for associated primitives
					if (indices[i] == curr_indice) {
						uint32_t i_mod_3 = (i % 3),
							grp = i - i_mod_3;
						grps.push_back (grp); // note: we are assuming model is made up of triangles, so grps of 3
						indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 1)%3)]); // for anticlockwise iteration
						indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 2)%3)]); // for anticlockwise iteration
					}
				}
				if (indices_of_neighbouring_vertives.empty ()) continue;
				std::vector<uint32_t> ring{ indices_of_neighbouring_vertives[0], indices_of_neighbouring_vertives[1] };
				bool keep_adding = true;
				while (keep_adding && !indices_of_neighbouring_vertives.empty ()) {
					keep_adding = false;

					// forward chain extension
					// 0-----0 - - 0 <-to_find_this
					//       ^-is_common_vertex
					for (auto itr = indices_of_neighbouring_vertives.begin (); itr != indices_of_neighbouring_vertives.end (); itr += 2) { // every odd position
						if (ring.back () == *itr) { // found shared vertex
							ring.push_back (*(itr+1)); // extend ring
							indices_of_neighbouring_vertives.erase (itr+1);
							indices_of_neighbouring_vertives.erase (itr);
							keep_adding = true;
							break;
						}
					}
					// backward chain extension
					// to_find_this->0 - - 0-----0
					// is_common_vertex ---^
					for (size_t i = 1; i < indices_of_neighbouring_vertives.size (); i += 2) { // every odd position
						if (ring.front () == indices_of_neighbouring_vertives[i]) { // found shared vertex
							ring.insert (ring.begin (), indices_of_neighbouring_vertives[i-1]); // extend ring
							auto itr = indices_of_neighbouring_vertives.begin () + (i-1);
							indices_of_neighbouring_vertives.erase (itr+1);
							indices_of_neighbouring_vertives.erase (itr);
							keep_adding = true;
							break;
						}
					}
				}

				auto curvature_along_edge = [&](uint32_t curr_vertex_index, uint32_t other_vertex_index) -> float {
					glm::vec3 nrml1 = posn_and_normals[curr_vertex_index].second;
					glm::vec3 nrml2 = posn_and_normals[other_vertex_index].second;
					glm::vec3 coord1 = posn_and_normals[curr_vertex_index].first;
					glm::vec3 coord2 = posn_and_normals[other_vertex_index].first;
					float _dot = glm::dot (nrml2 - nrml1, coord2 - coord1);
			#if MODE_DEBUG
					out_stream << _dot << ' ' << nrml2 << ' ' << nrml1 << ' ' << coord2 << ' ' << coord1 << '\n';
			#endif
					return _dot/glm::length2 (coord2 - coord1);
				};
			#if MODE_DEBUG
				out_stream << "dot(norml_diff,coord_diff), norml2(vec3), norml1(vec3), coord2(vec3), coord1(vec3):\n";
			#endif
				std::vector<float> ring_curvatures;
				ring_curvatures.reserve (ring.size ());
				for (auto &other : ring) {
					ring_curvatures.push_back (curvature_along_edge (curr_indice, other));
				}
			#if MODE_DEBUG
				out_stream << "Ring, ring-curve, coord: \n";
				for (uint32_t i = 0; i < ring.size (); i++) {
					glm::vec3 vertex = posn_and_normals[ring[i]].first;
					out_stream << ring[i] << ' ' << ring_curvatures[i] << '{' << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << '}' << '\n';
				}
				//out_stream << '\n';
			#endif
				auto angle_between_edges = [&](uint32_t curr_vertex_index, uint32_t other1_vertex_index, uint32_t other2_vertex_index) -> float {
					glm::vec3 edge1 = glm::normalize (posn_and_normals[other1_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					glm::vec3 edge2 = glm::normalize (posn_and_normals[other2_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					float e1e2sin_theta = glm::length (glm::cross (edge1, edge2));
					return asinf (MIN (1.0, e1e2sin_theta));
				};
				float angle_accumulator = 0.0f;
				float curvature = 0.0f;
				for (uint32_t i = 0, __ring_size = ring.size () - 1; i < __ring_size; i++) { // now this looks ugly
					float angle = angle_between_edges (curr_indice, ring[i], ring[i+1]);
					angle_accumulator += angle;
					curvature += (angle * (ring_curvatures[i] + ring_curvatures[i+1]));
				}
				curvatureAroundVertex = curvature/(2.0*angle_accumulator);
			
			#if MODE_DEBUG
				out_stream << "total angle: " << angle_accumulator << '\n' << " idx: " << curr_indice << " curvature: " << curvatureAroundVertex << '\n';
				out_stream << "currvertex: " << posn_and_normals[curr_indice].first << '\n' << '\n';
			#endif
			}
			mutex_curvature_push.lock ();
			curvatures.push_back (curvatureAroundVertex);
			max_curvature = MAX (curvatureAroundVertex, max_curvature);
			min_curvature = MIN (curvatureAroundVertex, min_curvature);
			mutex_curvature_push.unlock ();
		}
		float min_max_curvature_diff_2 = (max_curvature - min_curvature)*0.5;
	#if MODE_DEBUG
		mutex_ofs.lock ();
		std::cout << '\n';
		ofs << out_stream.str();
		mutex_ofs.unlock ();
	#endif
		for (size_t i = start; i < curvatures.size (); i += stride) {
			float curvature = curvatures[i];
			curvature -= min_curvature;
			curvature /= min_max_curvature_diff_2;

			if (curvature > 1.0) {
				curvature -= 1.0;
				curvature_diffuse_color[i] = glm::vec3 (1.0 - curvature, curvature, 0);
			} else {
				curvature_diffuse_color[i] = glm::vec3 (0, 1.0 - curvature, curvature);
			}
		}
	};
	std::vector<std::thread> threads;
	threads.reserve (std::thread::hardware_concurrency () - 1);
	for (uint32_t i = 1; i < std::thread::hardware_concurrency (); i++)
		threads.emplace_back(std::move(std::thread (mean_curvature_func, i, std::thread::hardware_concurrency ())));
	mean_curvature_func (0, std::thread::hardware_concurrency ());
	for (auto &ref : threads)
		ref.join ();
#if MODE_DEBUG
	ofs.close ();
#endif
	return true;
}
bool MeanCurvature2Calculate (const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &curvature_diffuse_color)
{
	float max_curvature = -std::numeric_limits<float>::max ();
	float min_curvature = std::numeric_limits<float>::max ();
	std::vector<glm::vec3> array_K_Xi;
	array_K_Xi.reserve (posn_and_normals.size ());
	std::vector<float> array_K_h; // mean curvature value
	array_K_h.reserve (posn_and_normals.size ());

#if MODE_DEBUG
	const char *fn = "curvatureResults.txt";
	std::ofstream ofs (fn);
	if (ofs.bad ()) {
		std::cout << "File creation error - " << fn << '\n';
		__debugbreak ();
	}
		std::mutex mutex_ofs;
#endif

	std::mutex mutex_curvature_push, mutex_cout;
	std::atomic<size_t> vertices_processed = 0;


	auto mean_curvature_func = [&](uint32_t start, uint32_t stride) {

	#if MODE_DEBUG
		std::ostringstream out_stream;
		out_stream << std::fixed << std::setprecision (8);
	#endif

		for (size_t curr_indice = start; curr_indice < posn_and_normals.size (); curr_indice += stride) // repeat for every vertex
		{
			mutex_cout.lock ();
			std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size ();
			vertices_processed++;
			mutex_cout.unlock ();

			glm::vec3 K_Xi = glm::vec3 (0);
			{
				std::vector<uint32_t> ring;
				{
					std::vector<uint32_t> indices_of_neighbouring_vertives; // bucket of neighboring vertices

					for (size_t i = 0; i < indices.size (); i++) { // Scan for associated primitives
						if (indices[i] == curr_indice) {
							uint32_t i_mod_3 = (i % 3),
								grp = i - i_mod_3;// note: we are assuming model is made up of triangles, so grps of 3
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 1)%3)]); // for anticlockwise iteration
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 2)%3)]); // for anticlockwise iteration
						}
					}
					if (indices_of_neighbouring_vertives.empty ()) continue;

					ring.push_back (indices_of_neighbouring_vertives[0]);
					ring.push_back (indices_of_neighbouring_vertives[1]);
					bool keep_adding = true;
					while (keep_adding && !indices_of_neighbouring_vertives.empty ()) {
						keep_adding = false;

						// forward chain extension
						// 0-----0 - - 0 <-to_find_this
						//       ^-is_common_vertex
						for (auto itr = indices_of_neighbouring_vertives.begin (); itr != indices_of_neighbouring_vertives.end (); itr += 2) { // every odd position
							if (ring.back () == *itr) { // found shared vertex
								ring.push_back (*(itr+1)); // extend ring
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}
						// backward chain extension
						// to_find_this->0 - - 0-----0
						// is_common_vertex ---^
						for (size_t i = 1; i < indices_of_neighbouring_vertives.size (); i += 2) { // every odd position
							if (ring.front () == indices_of_neighbouring_vertives[i]) { // found shared vertex
								ring.insert (ring.begin (), indices_of_neighbouring_vertives[i-1]); // extend ring
								auto itr = indices_of_neighbouring_vertives.begin () + (i-1);
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}

					}
				}


				auto angle_between_edges = [&](uint32_t curr_vertex_index, uint32_t other1_vertex_index, uint32_t other2_vertex_index) -> float {
					glm::vec3 edge1 = glm::normalize (posn_and_normals[other1_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					glm::vec3 edge2 = glm::normalize (posn_and_normals[other2_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					float e1e2sin_theta = glm::length (glm::cross (edge1, edge2));
					return asinf (MIN (1.0, e1e2sin_theta));
				};
				auto cotf = [](float angle_in_rad) -> float { return (angle_in_rad < glm::half_pi<float> ()) ? tanf (glm::half_pi<float> () - angle_in_rad) : -tanf (angle_in_rad - glm::half_pi<float> ()); };
				auto sqr = [](float val) -> float { return val*val; };
				//      /\X
				//     /  \
				//   Q/____\R
				float A_mixed = 0;
				std::vector<float> aij_bij;
				aij_bij.reserve (ring.size ()*2 - 3);
				for (uint32_t i = 1; i < ring.size (); i++) {
					float Q = angle_between_edges (ring[i-1], ring[i], curr_indice)
						, R = angle_between_edges (ring[i], curr_indice, ring[i-1])
						, X = angle_between_edges (curr_indice, ring[i], ring[i-1]);
					float XR2_cot_Q, XQ2_cot_R;
					{
						glm::vec3 XR_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i-1]].first
							, XQ_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i]].first;
						float XR2 = glm::length2 (XR_diff), XQ2 = glm::length2 (XQ_diff), cot_Q, cot_R;

						XR2_cot_Q = XR2*cotf (Q), XQ2_cot_R = XR2*cotf (R);
					}

					if (Q < glm::half_pi<float> () && R < glm::half_pi<float> () && X < glm::half_pi<float> ()) { // Acute △
						float A_voronoi = (XR2_cot_Q + XQ2_cot_R)*0.125; // (1/8)*((X-R)^2 * cotf(∠Q)  +  (X-Q)^2 * cotf(∠R))
						A_mixed += A_voronoi;
					} else {
						float Area_T = (XR2_cot_Q/sqr (sinf (R)) + XQ2_cot_R/sqr (sinf (Q)))*0.5;
						if (X >= glm::half_pi<float> ())
							A_mixed += Area_T*0.5;
						else
							A_mixed += Area_T*0.25;
					}

					if (!aij_bij.empty ()) {
						aij_bij.push_back (R);
						aij_bij.push_back (Q);
					} else aij_bij.push_back (Q);
				}
				aij_bij.pop_back (); // last one is not part of pairs
				// final size() = ring.size ()*2 - 4
				for (float &val: aij_bij)
					val = cotf (val);
				glm::vec3 sigma_j_in_ring = glm::vec3 (0.0f);
				for (uint32_t i = 1, __nringsize = ring.size () - 1; i < __nringsize; i++) {
					uint32_t idx = ((i - 1) << 1); // (i - 1)*2
					float cot_aij = aij_bij[idx];
					float cot_bij = aij_bij[idx+1];
				glm::vec3 edge = posn_and_normals[curr_indice].first - posn_and_normals[ring[i]].first;
					sigma_j_in_ring += edge*(cot_aij + cot_bij);
				}
				K_Xi = (sigma_j_in_ring/A_mixed)*0.5f;
			}
			mutex_curvature_push.lock ();
			array_K_Xi.push_back (K_Xi);
			float K_h = glm::length (K_Xi)*0.5;
			array_K_h.push_back (K_h);
			max_curvature = MAX (K_h, max_curvature);
			min_curvature = MIN (K_h, min_curvature);
			mutex_curvature_push.unlock ();

		#if MODE_DEBUG
			out_stream << " idx: " << curr_indice << " mean_curvature_val: " << K_h << " Mean Curvature Norml: " << K_Xi << '\n';
			out_stream << "currvertex: " << posn_and_normals[curr_indice].first << '\n' << '\n';
		#endif
		}
		float min_max_curvature_diff_by_2 = (max_curvature - min_curvature)*0.5;
	#if MODE_DEBUG
		mutex_ofs.lock ();
		std::cout << '\n';
		ofs << out_stream.str ();
		mutex_ofs.unlock ();
	#endif
		//for (size_t i = start; i < array_K_Xi.size (); i += stride) {
		//	curvature_diffuse_color[i] = array_K_Xi[i];
		//}
		for (size_t i = start; i < array_K_Xi.size (); i += stride) {
			float curvature = array_K_h[i];
			curvature -= min_curvature;
			curvature /= min_max_curvature_diff_by_2;

			if (curvature > 1.0) {
				curvature -= 1.0;
				curvature_diffuse_color[i] = glm::vec3 (1.0 - curvature, curvature, 0);
			} else {
				curvature_diffuse_color[i] = glm::vec3 (0, 1.0 - curvature, curvature);
			}
		}
	};
	std::vector<std::thread> threads;
	threads.reserve (std::thread::hardware_concurrency () - 1);
	for (uint32_t i = 1; i < std::thread::hardware_concurrency (); i++)
		threads.emplace_back (std::move (std::thread (mean_curvature_func, i, std::thread::hardware_concurrency ())));
	mean_curvature_func (0, std::thread::hardware_concurrency ());
	for (auto &ref : threads)
		ref.join ();
#if MODE_DEBUG
	ofs.close ();
#endif
	return true;
}