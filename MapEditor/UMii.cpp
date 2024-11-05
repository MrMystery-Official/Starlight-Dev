#include "UMii.h"

#include "Byml.h"
#include "Logger.h"
#include <iostream>

UMii::UMii(std::vector<unsigned char> BymlBytes)
{
	BymlFile UMiiByml(BymlBytes);

	if (UMiiByml.HasChild("SexAge"))
	{
		this->SexAge = UMiiByml.GetNode("SexAge")->GetValue<std::string>();
	}
	if (UMiiByml.HasChild("Race"))
	{
		this->Race = UMiiByml.GetNode("Race")->GetValue<std::string>();
	}

	BymlFile::Node* BodyDict = UMiiByml.GetNode("Body");
	if (BodyDict != nullptr)
	{
		this->Type = BodyDict->GetChild("Type")->GetValue<std::string>();
		
		if (BodyDict->HasChild("Number"))
		{
			this->Number = BodyDict->GetChild("Number")->GetValue<std::string>();
			this->Number = this->Number.substr(6);
			if (this->Number.length() == 1)
				Number = "00" + Number;
			else if (this->Number.length() == 2)
				Number = "0" + Number;
		}
	}

	/* Constructing model */
	std::string ModelRace = Race == "Korog" ? "Korogu" : Race;
	BfresFile* BodyModelPtr = BfresLibrary::GetModel("UMii_" + ModelRace + "_Body" + Type + "_" + SexAge + "_" + Number + "." + "UMii_" + ModelRace + "_Body" + Type + "_" + SexAge + "_" + Number);
	if (BodyModelPtr != nullptr)
	{
		GLBfres* GLModel = GLBfresLibrary::GetModel(BodyModelPtr);
		if (GLModel != nullptr && !GLModel->mBfres->Models.Nodes.empty())
		{
			this->m_ModelElements.push_back(ModelElement{
				.Model = GLModel
				}); //Body
		}
	}

	/*
	BfresFile* BodyModelPtr = BfresLibrary::GetModel("UMii_" + ModelRace + "_Body" + Type + "_" + SexAge + "_" + Number + "." + "UMii_" + ModelRace + "_Body" + Type + "_" + SexAge + "_" + Number);

	BymlFile::Node* ShapeDict = UMiiByml.GetNode("Shape");
	if (ShapeDict != nullptr)
	{
		int32_t SkinColor = ShapeDict->GetChild("ShapeColor")->GetValue<int32_t>();
		Vector3F NewColor(255, 0, 0);

		for (BfresFile::Material& Material : BodyModelPtr->GetModels()[0].Materials)
		{
			std::cout << "MAT: " << Material.Name << std::endl;
			if (Material.Name == "Mt_Skin")
			{
				std::string Name = "";
				for (auto& [Key, Val] : TextureToGoLibrary::Textures)
				{
					if (&Val == Material.Textures[0].Texture)
					{
						Name = Key;
						std::cout << "NAME_ " << Name << std::endl;
						break;
					}
				}

				for (unsigned char& Pixel : TextureToGoLibrary::Textures[Name].GetPixels())
				{
					Pixel = 0;
				}

				std::cout << "REC\n";
				GLTextureLibrary::Textures.erase(&TextureToGoLibrary::Textures[Name]);
				BodyModelPtr->CreateOpenGLObjects();
				break;
			}
		}
	}

	this->m_ModelElements.push_back(ModelElement{
	.Model = BodyModelPtr
		}); //Body
		*/
}

void UMii::Draw(Vector3F Translate, Vector3F Rotate, Vector3F Scale, Shader* Shader, bool Picking, Camera* CameraView)
{
	for (UMii::ModelElement& Element : this->m_ModelElements)
	{
		/*
		if (Element.Model->Models.Nodes.empty())
		{
			Element.Model = BfresLibrary::GetModel("Default");
		}
		*/
		
		std::vector<glm::mat4> Instances(1);

		glm::mat4& GLMModel = Instances[0];
		GLMModel = glm::mat4(1.0f); // Identity matrix

		GLMModel = glm::translate(GLMModel, glm::vec3(Translate.GetX(), Translate.GetY(), Translate.GetZ()));

		GLMModel = glm::rotate(GLMModel, glm::radians(Rotate.GetZ()), glm::vec3(0.0, 0.0f, 1.0));
		GLMModel = glm::rotate(GLMModel, glm::radians(Rotate.GetY()), glm::vec3(0.0f, 1.0, 0.0));
		GLMModel = glm::rotate(GLMModel, glm::radians(Rotate.GetX()), glm::vec3(1.0, 0.0f, 0.0));

		GLMModel = glm::scale(GLMModel, glm::vec3(Scale.GetX(), Scale.GetY(), Scale.GetZ()));

		Element.Model->Draw(Instances, Shader);
	}
}