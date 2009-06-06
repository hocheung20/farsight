/*
  Farsight ToolKit 3D Viewer: 
  v1: implements render window functionality
    render window creation in "View3D::RenderWin"
    line objects mapped and actors created in "View3D::LineAct"
    actor added and window created in "View3D::addAct"
  v2: add picking functionality:
    access to observing a pick event is in "View3D::interact"
    the interacter calls for "View3D::PickPoint"
  v3: include contourFilter and rayCast renderers
  v4: converted to Qt, member names changed to fit "VTK style" more closely.
*/
#include "View3D.h"
#include "View3DHelperClasses.h"

View3D::View3D(int argc, char **argv)
{
  this->tobj = new TraceObject;
  int num_loaded = 0;
  this->Volume=0;
  // load as many files as possible. Provide offset for differentiating types
	for(int counter=1; counter<argc; counter++)
	  {
		int len = strlen(argv[counter]);
		if(strcmp(argv[counter]+len-3,"swc")==0)
		  {
			printf("I detected swc\n");
			this->tobj->ReadFromSWCFile(argv[counter]);
		   }
		else if (strcmp(argv[counter]+len-3,"xml")==0)
		  {
			printf("I detected xml\n");
			this->tobj->ReadFromRPIXMLFile(argv[counter]);
		  }
		else if( strcmp(argv[counter]+len-3,"tks")==0)
		  {
			printf("I detected tks\n");
			this->tobj->ReadFromFeatureTracksFile(argv[counter],num_loaded);
		  }
		else if( strcmp(argv[counter]+len-3,"tif")==0 ||
             strcmp(argv[counter]+len-3, "pic")==0)
		  {
			printf("I detected a tif file\n");
			this->rayCast(argv[counter]);
			//this->AddVolumeSliders();
		  }
		num_loaded++;
	  }
this->QVTK = 0;
this->gapTol = 2;
this->gapMax = 15;
this->smallLine = 5;
this->SelectColor =.3;
this->lineWidth= 2;
this->Initialize();
}

View3D::~View3D()
{
  if(this->QVTK)
    {
    delete this->QVTK;
    }
  delete this->tobj;
}

void View3D::Initialize()
{
  this->CreateGUIObjects();
  this->CreateLayout();
  this->CreateInteractorStyle();
  this->CreateActors();
  this->resize(640, 480);
  this->setWindowTitle(tr("Trace editor"));
  this->QVTK->GetRenderWindow()->Render();
}

/*  set up the components of the interface */
void View3D::CreateGUIObjects()
{
  //Setup a QVTK Widget for embedding a VTK render window in Qt.
  this->QVTK = new QVTKWidget(this);
  this->Renderer = vtkRenderer::New();
  this->QVTK->GetRenderWindow()->AddRenderer(this->Renderer);

  //Setup the buttons that the user will use to interact with this program. 
  this->ListButton = new QPushButton("&list selections", this);
  this->ClearButton = new QPushButton("&clear selection", this);
  this->DeleteButton = new QPushButton("&delete trace", this);
  this->MergeButton = new QPushButton("&merge traces", this);
  this->SplitButton = new QPushButton("&split trace", this);
  this->FlipButton = new QPushButton("&flip trace direction", this);
  this->WriteButton = new QPushButton("&write to .swc file", this);
  this->SettingsButton = new QPushButton("&edit settings", this);
  this->AutomateButton = new QPushButton("&Automatic Selection", this);

  //Setup the tolerance settings editing window
  this->SettingsWidget = new QWidget();
  QIntValidator *intValidator = new QIntValidator(1, 100, this->SettingsWidget);
  QDoubleValidator *doubleValidator =
    new QDoubleValidator(0, 100, 2, this->SettingsWidget);
  this->MaxGapField = new QLineEdit(this->SettingsWidget);
  this->MaxGapField->setValidator(intValidator);
  this->GapToleranceField = new QLineEdit(this->SettingsWidget);
  this->GapToleranceField->setValidator(intValidator);
  this->LineLengthField = new QLineEdit(this->SettingsWidget);
  this->LineLengthField->setValidator(doubleValidator);
  this->ColorValueField = new QLineEdit(this->SettingsWidget);
  this->ColorValueField->setValidator(doubleValidator);
  this->LineWidthField = new QLineEdit(this->SettingsWidget);
  this->LineWidthField->setValidator(doubleValidator);
  this->ApplySettingsButton = new QPushButton("Apply", this->SettingsWidget);
  this->CancelSettingsButton = new QPushButton("Cancel", this->SettingsWidget);

  //Setup the connections
  connect(this->ListButton, SIGNAL(clicked()), this, SLOT(ListSelections()));
  connect(this->ClearButton, SIGNAL(clicked()), this, SLOT(ClearSelection()));
  connect(this->DeleteButton, SIGNAL(clicked()), this, SLOT(DeleteTraces()));
  connect(this->MergeButton, SIGNAL(clicked()), this, SLOT(MergeTraces()));
  connect(this->SplitButton, SIGNAL(clicked()), this, SLOT(SplitTraces()));
  connect(this->FlipButton, SIGNAL(clicked()), this, SLOT(FlipTraces()));
  connect(this->WriteButton, SIGNAL(clicked()), this, SLOT(WriteToSWCFile()));
 connect(this->AutomateButton, SIGNAL(clicked()), this, SLOT(SLine()));
  connect(this->SettingsButton, SIGNAL(clicked()), this,
    SLOT(ShowSettingsWindow()));
  connect(this->ApplySettingsButton, SIGNAL(clicked()), this,
    SLOT(ApplyNewSettings()));
  connect(this->CancelSettingsButton, SIGNAL(clicked()), this,
    SLOT(HideSettingsWindow()));
}

void View3D::CreateLayout()
{
  //layout for the main window
	QGridLayout *buttonLayout = new QGridLayout();
	buttonLayout->addWidget(this->AutomateButton, 1, 0);
	buttonLayout->addWidget(this->ListButton, 2, 0);
	buttonLayout->addWidget(this->ClearButton, 3, 0);
	buttonLayout->addWidget(this->DeleteButton, 4, 0);
	buttonLayout->addWidget(this->MergeButton, 5, 0);
	buttonLayout->addWidget(this->SplitButton, 6, 0);
	buttonLayout->addWidget(this->FlipButton, 7, 0);

	buttonLayout->addWidget(this->SettingsButton, 9, 0);
	buttonLayout->addWidget(this->WriteButton, 10, 0);
	buttonLayout->setSpacing(10);
	QGridLayout *viewerLayout = new QGridLayout(this);
	viewerLayout->addWidget(this->QVTK, 0, 1);
	viewerLayout->addLayout(buttonLayout, 0, 0);

  //layout for the settings window
  QGridLayout *settingsLayout = new QGridLayout(this->SettingsWidget);
  QLabel *maxGapLabel = new QLabel(tr("Maximum gap length:"));
  settingsLayout->addWidget(maxGapLabel, 0, 0);
  settingsLayout->addWidget(this->MaxGapField, 0, 1);
  QLabel *gapToleranceLabel = new QLabel(tr("Gap length tolerance:"));
  settingsLayout->addWidget(gapToleranceLabel, 1, 0);
  settingsLayout->addWidget(this->GapToleranceField, 1, 1);
  QLabel *lineLengthLabel = new QLabel(tr("Small line length:"));
  settingsLayout->addWidget(lineLengthLabel, 2, 0);
  settingsLayout->addWidget(this->LineLengthField, 2, 1);
  QLabel *colorValueLabel = new QLabel(tr("Color value RGB scalar 0 to 1:"));
  settingsLayout->addWidget(colorValueLabel, 3, 0);
  settingsLayout->addWidget(this->ColorValueField, 3, 1);
  QLabel *lineWidthLabel = new QLabel(tr("Line width:"));
  settingsLayout->addWidget(lineWidthLabel, 4, 0);
  settingsLayout->addWidget(this->LineWidthField, 4, 1);
  settingsLayout->addWidget(this->ApplySettingsButton, 5, 0);
  settingsLayout->addWidget(this->CancelSettingsButton, 5, 1);
}

void View3D::CreateInteractorStyle()
{
  this->Interactor = this->QVTK->GetRenderWindow()->GetInteractor();
	//keep mouse command observers, but change the key ones
  this->keyPress = vtkCallbackCommand::New();
  this->keyPress->SetCallback(HandleKeyPress);
  this->keyPress->SetClientData(this);

  this->Interactor->RemoveObservers(vtkCommand::KeyPressEvent);
	this->Interactor->RemoveObservers(vtkCommand::KeyReleaseEvent);
	this->Interactor->RemoveObservers(vtkCommand::CharEvent);
	this->Interactor->AddObserver(vtkCommand::KeyPressEvent, this->keyPress);

  //use trackball control for mouse commands
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkInteractorStyleTrackballCamera::New();
  this->Interactor->SetInteractorStyle(style);
  this->CellPicker = vtkCellPicker::New();
  this->CellPicker->SetTolerance(0.004);
  this->Interactor->SetPicker(this->CellPicker);
  this->isPicked = vtkCallbackCommand::New();
  this->isPicked->SetCallback(PickCell);

  //isPicked caller allows observer to intepret click 
  this->isPicked->SetClientData(this);            
  this->Interactor->AddObserver(vtkCommand::RightButtonPressEvent,isPicked);
}

void View3D::CreateActors()
{
  this->LineActor = vtkActor::New();
  this->LineMapper = vtkPolyDataMapper::New();
  this->UpdateLineActor();
  this->LineActor->SetPickable(1);
  this->Renderer->AddActor(this->LineActor);

  this->UpdateBranchActor();
  this->Renderer->AddActor(this->BranchActor);
 
  if(this->Volume!=NULL)
  {
	  this->Renderer->AddVolume(this->Volume);
	  this->AddVolumeSliders();
  }
  //sphere is used to mark the picks
  this->CreateSphereActor();
  Renderer->AddActor(this->SphereActor);
}

void View3D::CreateSphereActor()
{
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetRadius(3);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->Sphere->GetOutput());
  this->SphereMapper->GlobalImmediateModeRenderingOn();

  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);
  this->SphereActor->GetProperty()->SetOpacity(.3);
  this->SphereActor->VisibilityOff();
  this->SphereActor->SetPickable(0);            //dont want to pick the sphere itself
}

/* update trace data */
void View3D::UpdateLineActor()
{
  this->poly_line_data = this->tobj->GetVTKPolyData();
  this->LineMapper->SetInput(this->poly_line_data);
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->GetProperty()->SetColor(0,1,0);
  this->LineActor->GetProperty()->SetPointSize(2);
  this->LineActor->GetProperty()->SetLineWidth(lineWidth);
}

void View3D::UpdateBranchActor()
{
	this->poly = tobj->generateBranchIllustrator();
	this->polymap = vtkSmartPointer<vtkPolyDataMapper>::New();
	polymap->SetInput(this->poly);
	this->BranchActor = vtkSmartPointer<vtkActor>::New();
	this->BranchActor->SetMapper(this->polymap);
	this->BranchActor->SetPickable(0);
	//Renderer->AddActor(BranchActor);
	//BranchActor->Print(std::cout);

}
void View3D::AddPointsAsPoints(std::vector<TraceBit> vec)
{
	vtkSmartPointer<vtkCubeSource> cube_src = vtkSmartPointer<vtkCubeSource>::New();
	cube_src->SetBounds(-0.2,0.2,-0.2,0.2,-0.2,0.2);
  vtkSmartPointer<vtkPolyData> point_poly = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points=vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cells=vtkSmartPointer<vtkCellArray>::New();
  for(unsigned int counter=0; counter<vec.size(); counter++)
  {
    int return_id = points->InsertNextPoint(vec[counter].x,vec[counter].y,vec[counter].z);
    cells->InsertNextCell(1);
    cells->InsertCellPoint(return_id);
  }
  printf("About to create poly\n");
  point_poly->SetPoints(points);
  point_poly->SetVerts(cells);
  vtkSmartPointer<vtkGlyph3D> glyphs = vtkSmartPointer<vtkGlyph3D>::New();
  glyphs->SetSource(cube_src->GetOutput());
  glyphs->SetInput(point_poly);
  vtkSmartPointer<vtkPolyDataMapper> cubemap = vtkSmartPointer<vtkPolyDataMapper>::New();
  cubemap->SetInput(glyphs->GetOutput());
  cubemap->GlobalImmediateModeRenderingOn();
  vtkSmartPointer<vtkActor> cubeact = vtkSmartPointer<vtkActor>::New();
  cubeact->SetMapper(cubemap);
  cubeact->SetPickable(0);
  cubeact->GetProperty()->SetPointSize(5);
  cubeact->GetProperty()->SetOpacity(.5);
  Renderer->AddActor(cubeact);

}

void View3D::HandleKeyPress(vtkObject* caller, unsigned long event,
                            void* clientdata, void* callerdata)
{
  View3D* view = (View3D*)clientdata;
  char key = view->Interactor->GetKeyCode();
  switch (key)
    {
    case 'l':
      view->ListSelections();
      break;

    case 'c':
      view->ClearSelection();
      break;

    case 'd':
      view->DeleteTraces();
      break;
    
	  case 'm':
      view->MergeTraces();
      break;
    
    case 's':
      view->SplitTraces();
      break;

    case 'f':
      view->FlipTraces();
      break;

	  case 'w':
      view->WriteToSWCFile();
	  break;

	  case 't':
      view->ShowSettingsWindow();
      break;

	  case 'a':
		  view->SLine();
		  /*std::cout<<"select small lines\n";
		  view->tobj->FindMinLines(view->smallLine);
		  view->Rerender();*/
	    break;

    case '-':
      if(view->IDList.size()>=1)
        {
        TraceLine* tline=reinterpret_cast<TraceLine*>(
          view->tobj->hashc[view->IDList[view->IDList.size()-1]]);
        view->HighlightSelected(tline, tline->getTraceColor()-.25);
        view->IDList.pop_back();
        cout<< " These lines: ";
        for (unsigned int i = 0; i < view->IDList.size(); i++)
          {
          cout<<  "\t"<<view->IDList[i];   
          } 
        cout<< " \t are selected \n" ;   
        }
      break;

    default:
      break;
    }
}
void View3D::SLine()
{
	int numLines, i;
	this->tobj->FindMinLines(this->smallLine);
	numLines= this->tobj->SmallLines.size();
	this->Rerender();
	QMessageBox Myquestion;
	Myquestion.setText("Number of selected small lines:  " 
		+ QString::number(numLines));
	Myquestion.setInformativeText("Delete these small lines?" );
	Myquestion.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
	Myquestion.setDefaultButton(QMessageBox::Yes);
	int ret = Myquestion.exec();
	switch (ret) 
	{	
	case QMessageBox::Yes:
	{
		for (i=0;i<numLines-1;i++)
		{	//std::cout << "Deleted line:" << i<< std::endl;					
			//this->tobj->SmallLines[i]->Getstats();
			this->tobj->RemoveTraceLine(this->tobj->SmallLines[i]);
		}
		this->tobj->SmallLines.clear();
	}
	break;
	case QMessageBox::No:
	 {
		 this->tobj->SmallLines.clear();
	 }
	 break;
	}
	this->Rerender();
}
void View3D::Rerender()
{
  this->SphereActor->VisibilityOff();
  this->IDList.clear();
  this->Renderer->RemoveActor(this->BranchActor);
  this->UpdateLineActor();
  this->UpdateBranchActor();
  this->Renderer->AddActor(this->BranchActor);
  this->QVTK->GetRenderWindow()->Render();
}

void View3D::ListSelections()
{
  QLabel *selectionInfo = new QLabel();
  QString listText;

  if (this->IDList.size()<= 0)
    {
    listText = tr("No traces selected");
    }
  else
    {
    listText += QString::number(this->IDList.size()) + " lines are selected:\n";
    for (unsigned int i = 0; i < this->IDList.size(); i++)
      {
      listText += QString::number(this->IDList[i]) + "\n";   
      } 
    }
  selectionInfo->setText(listText);
  selectionInfo->show();
}

void View3D::ClearSelection()
{
  if (this->IDList.size()<= 0)
    {
    std::cout<<  "Nothing Selected \n";
    }
  else
    {
    this->IDList.clear();
    std::cout<< "cleared list\n";
    this->Rerender();
    }
}

void View3D::DeleteTraces()
{
  if(this->IDList.size()>=1)
    {
    std::cout<<"selected lines \n";
    for (unsigned int i = 0; i < this->IDList.size(); i++)
      {
      std::cout<<  "\t"<<this->IDList[i];
      this->DeleteTrace(reinterpret_cast<TraceLine*>(this->tobj->hashc[this->IDList[i]])); 
      } 
    std::cout<< " \t deleted" <<std::endl;
    this->Rerender();
    }
  else
    {
    std::cout<<  "Nothing to Delete \n";
    }
}

void View3D::MergeTraces()
{
  if(this->IDList.size()>=1)
    {		  
    std::sort(this->IDList.begin(), this->IDList.end());
    std::reverse(this->IDList.begin(), this->IDList.end());
    int numTrace = this->IDList.size();		
    int i,j, s=0, exist =0;
    std::vector<TraceLine*> traceList;
    //std::cout<< "elements passed \t" << numTrace << std::endl;
    for (i = 0;i<numTrace; i++)
      {
      traceList.push_back( reinterpret_cast<TraceLine*>(
        this->tobj->hashc[this->IDList[i]]));
      s=traceList.size()-1;
      exist = 0;
      if (traceList.size()>0)
        {
        j = 0;
        while ( (j < s)&&(exist==0))
          {	
          if (traceList[s]->GetId()== traceList[j]->GetId())
            {
            traceList.pop_back();	
            exist = 1;
            }
          j++;			
          }
        }
      }
	//this->Rerender();
    this->MinEndPoints(traceList);
    this->Rerender();
    }
  else
    {
    //std::cout<<  "Nothing to merge \n";
		this->MinEndPoints(this->tobj->GetTraceLines());
		this->Rerender();
    }
}

void View3D::SplitTraces()
{
  if(this->IDList.size()>=1)
    {
    std::unique(this->IDList.begin(), this->IDList.end());
    for(unsigned int i = 0; i < this->IDList.size(); i++)
      {
      this->tobj->splitTrace(this->IDList[i]);
      } 
    this->Rerender();
    }
  else
    {
    cout << "Nothing to split" << endl;
    }
}

void View3D::FlipTraces()
{
  if(this->IDList.size()>=1)
    {
    for(unsigned int i=0; i< this->IDList.size(); i++)
      {
      this->tobj->ReverseSegment(reinterpret_cast<TraceLine*>(
        this->tobj->hashc[this->IDList[i]]));
      }
    this->Rerender();
    }
}

void View3D::WriteToSWCFile()
{
  QString fileName = QFileDialog::getSaveFileName(
    this,
    tr("Save File"),
    "",
    tr("SWC Images (*.swc)"));
	this->tobj->WriteToSWCFile(fileName.toStdString().c_str());	
}

void View3D::ShowSettingsWindow()
{
  //make sure the values in the input fields are up-to-date
  this->MaxGapField->setText(QString::number(this->gapMax));
  this->GapToleranceField->setText(QString::number(this->gapTol));
  this->LineLengthField->setText(QString::number(this->smallLine));
  this->ColorValueField->setText(QString::number(this->SelectColor));
  this->LineWidthField->setText(QString::number(this->lineWidth));
  this->SettingsWidget->show();
}

void View3D::ApplyNewSettings()
{
  this->gapMax = this->MaxGapField->text().toInt();
  this->gapTol = this->GapToleranceField->text().toInt();
  this->smallLine = this->LineLengthField->text().toFloat();
  this->SelectColor = this->ColorValueField->text().toDouble();
  this->lineWidth = this->LineWidthField->text().toFloat();
  //this->Rerender();
  this->SettingsWidget->hide();
}

void View3D::HideSettingsWindow()
{
  this->SettingsWidget->hide();
}

void View3D::DeleteTrace(TraceLine *tline)
{
  std::vector<unsigned int> * vtk_cell_ids = tline->GetMarkers();

  vtkIdType ncells; vtkIdType *pts;
  for(unsigned int counter=0; counter<vtk_cell_ids->size(); counter++)
    {
    this->poly_line_data->GetCellPoints((vtkIdType)(*vtk_cell_ids)[counter],ncells,pts);
    pts[1]=pts[0];
    }
  std::vector<TraceLine*> *children = tline->GetBranchPointer();
  if(children->size()!=0)
    {
    for(unsigned int counter=0; counter<children->size(); counter++)
      {
      this->poly_line_data->GetCellPoints((*(*children)[counter]->GetMarkers())[0],ncells,pts);
      pts[1]=pts[0];
      this->tobj->GetTraceLinesPointer()->push_back((*children)[counter]);  
      (*children)[counter]->SetParent(NULL);
      }
    // remove the children now
    children->clear();
    }         //finds and removes children
  // remove from parent
  std::vector<TraceLine*>* siblings;
  if(tline->GetParent()!=NULL)
    {
    siblings=tline->GetParent()->GetBranchPointer();
	  if(siblings->size()==2)
	    {
      // its not a branch point anymore
      TraceLine *tother1;
      if(tline==(*siblings)[0])
        { 
        tother1 = (*siblings)[1];
        }
      else
        {
        tother1 = (*siblings)[0];
        }
      tother1->SetParent(NULL);
      siblings->clear();
      TraceLine::TraceBitsType::iterator iter1,iter2;
      iter1= tline->GetParent()->GetTraceBitIteratorEnd();
      iter2 = tother1->GetTraceBitIteratorBegin();
      iter1--;
		
      this->tobj->mergeTraces((*iter1).marker,(*iter2).marker);
      tline->SetParent(NULL);
      delete tline;
      return;
	    }
    }
  else
    {
    siblings = this->tobj->GetTraceLinesPointer();
    }
  std::vector<TraceLine*>::iterator iter = siblings->begin();
  std::vector<TraceLine*>::iterator iterend = siblings->end();
  while(iter != iterend)
    {
    if(*iter== tline)
      {
      siblings->erase(iter);
      break;
      }
    ++iter;
    }
  tline->SetParent(NULL);
}

bool View3D::setTol()
{
	bool change = false;
	char select=0;
	std::cout<<"Settings Configuration:\n gap (t)olerance:\t" <<gapTol
		<<"\ngap (m)ax:\t"<<gapMax
		<<"\n(s)mall line:\t"<< smallLine
		<<"\nselection (c)olor:\t"<<SelectColor
		<<"\nline (w)idth:\t"<<lineWidth
		<<"\n(e)nd edit settings\n";
	while (select !='e')
	{
		std::cout<< "select option:\t"; 
		std::cin>>select;
		switch(select)
		{
		case 'm':
			{
				int newMax;
				std::cout<< "maximum gap length\n";
				std::cin>>newMax;
				if (newMax!=gapMax)
				{
					gapMax=newMax;
					change= true;
				}
				break;
			}//end of 'm'
		case 't':
			{
				int newTol;
				std::cout<< "gap length tolerance\n";
				std::cin>>newTol;
				if (newTol!=gapTol)
				{
					gapTol=newTol;
					change= true;
				}
				break;
			}//end of 't'
		case 's':
			{
				float newSmall;
				std::cout<< "small line length\n";
				std::cin>>newSmall;
				if (newSmall!=smallLine)
				{
					smallLine=newSmall;
					change= true;
				}
				break;
			}// end of 's'
		case 'c':
			{
				double newColor;
				std::cout<< "color value RGB scalar 0 to 1\n";
				std::cin>>newColor;
				if (newColor!=SelectColor)
				{
					SelectColor=newColor;
					change= true;
				}
				break;
			}//end of 'c'
		case 'w':
			{
				float newWidth;
				std::cout<<"line Width\n";
				std::cin>>newWidth;
				if (newWidth!=lineWidth)
				{
					lineWidth=newWidth;
					change= true;
				}
				break;
			}
		}//end of switch
	}// end of while
	if (change== true)
	{std::cout<<"Settings Configuration are now:\n gap tollerance:\t" <<gapTol
		<<"\ngap max:\t"<<gapMax
		<<"\nsmall line:\t"<< smallLine
		<<"\nselection color:\t"<<SelectColor
		<<"\nline width:\t"<<lineWidth;
	}
	return change;
}
void View3D::PickCell(vtkObject* caller, unsigned long event, void* clientdata, void* callerdata)
{ /*  PickPoint allows fot the point id and coordinates to be returned 
  as well as adding a marker on the last picked point
  R_click to select point on line  */
  //printf("Entered pickCell\n");
  View3D* view = (View3D*)clientdata;       //acess to view3d class so view-> is used to call functions

  int *pos = view->Interactor->GetEventPosition();
  view->Interactor->GetPicker()->Pick(pos[0],pos[1],0.0,view->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
  vtkCellPicker *cell_picker = (vtkCellPicker *)view->Interactor->GetPicker();
  if (cell_picker->GetCellId() == -1) 
  {
    view->SphereActor->VisibilityOff();     //not working quite yet but sphere will move
  }
  else if(cell_picker->GetViewProp()!=NULL) 
  {
    //double selPt[3];
    double pickPos[3];
    view->CellPicker->GetPickPosition(pickPos);    //this is the coordinates of the pick
    //view->cell_picker->GetSelectionPoint(selPt);    //not so usefull but kept
    unsigned int cell_id = cell_picker->GetCellId();  
    view->IDList.push_back(cell_id);
    TraceLine *tline = reinterpret_cast<TraceLine*>(view->tobj->hashc[cell_id]);
	
	view->HighlightSelected(tline, view->SelectColor);
    tline->Getstats();              //prints the id and end coordinates to the command prompt 
    view->SphereActor->SetPosition(pickPos);    //sets the selector to new point
    view->SphereActor->VisibilityOn();      //deleteTrace can turn it off 
    view->poly_line_data->Modified();
	
  }
  view->QVTK->GetRenderWindow()->Render();             //update the render window
}

void View3D::MinEndPoints(std::vector<TraceLine*> traceList)
{
	unsigned int i,j, exist = 0, conflict = 0;
	std::vector<compTrace> compList;
	std::vector<compTrace> grayList;	//grayList is a list of all further possible merges within tolerance
	QMessageBox *MergeInfo = new QMessageBox;
	MergeInfo->setText("Merge Function");
	QPushButton *mergeAll;
	QPushButton *mergeNone;
	QString myText;	QString dtext;	QString grayText;
	myText+="Number of traces:\t" + QString::number(traceList.size());
	for (i=0;i<traceList.size()-1; i++)
	  {
		for (j=i+1; j<traceList.size(); j++)
		  {
			compTrace newComp;
			newComp.Trace1= traceList[i];
			newComp.Trace2= traceList[j];
			newComp.Trace1->EndPtDist(
        newComp.Trace2,newComp.endPT1, newComp.endPT2, newComp.dist, newComp.maxdist, newComp.angle);
			if(!(newComp.dist >= newComp.Trace1->GetSize()/gapTol) 
				&&	!(newComp.dist >= newComp.Trace2->GetSize()/gapTol) 
				&&	!(newComp.dist >= gapMax + gapMax/gapTol))
			  {
				//myText+="added comparison\n";
				compList.push_back(newComp);
			  }
			/*else
			{
				std::cout<<"distance "
				<< newComp.Trace1->GetId()
				<< " and " << newComp.Trace2->GetId()
				<<" is too large \n";
			}*/			
		  }
	}
	
	if (compList.size() >= 1)
	  {
		myText+="\tNumber of comparisons:\t" + QString::number(compList.size());
		i = 0, j = 0; int mergeCount = 0;
		while (i < compList.size() -1)
		  {
			exist = 0;
			while ((exist == 0)&&(j<compList.size()-1))
			  {
				j++;
				if (compList[i].Trace1->GetId()==compList[j].Trace1->GetId())
				  {
					if (compList[i].endPT1==compList[j].endPT1)
					  {/*
						myText+="Conflict:\t";	myText+=QString::number(compList[i].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[i].Trace2->GetId());
						myText+="\tand\t";		myText+=QString::number(compList[j].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[j].Trace2->GetId());	*/
						exist = 1;
            }
			  	}
				else if(compList[i].Trace1->GetId()==compList[j].Trace2->GetId())
				  {
					if (compList[i].endPT1==compList[j].endPT2)
					  {/*
						myText+="Conflict:\t";	myText+=QString::number(compList[i].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[i].Trace2->GetId());
						myText+="\tand\t";		myText+=QString::number(compList[j].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[j].Trace2->GetId());	*/
						exist = 1;
            }
				  }
				else if (compList[i].Trace2->GetId() == compList[j].Trace1->GetId())
  				{
					if (compList[i].endPT2==compList[j].endPT1)
					  {/*
						myText+="Conflict:\t";	myText+=QString::number(compList[i].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[i].Trace2->GetId());
						myText+="\tand\t";		myText+=QString::number(compList[j].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[j].Trace2->GetId());	*/					
						exist = 1;
            }
				  }
				else if(compList[i].Trace2->GetId() == compList[j].Trace2->GetId())
				  {
					if (compList[i].endPT2==compList[j].endPT2)
				  	{/*
						myText+="Conflict:\t";	myText+=QString::number(compList[i].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[i].Trace2->GetId());
						myText+="\tand\t";		myText+=QString::number(compList[j].Trace1->GetId());
						myText+="\tto\t";			myText+=QString::number(compList[j].Trace2->GetId());*/
						exist = 1;
            }
				  }
			  }
			if (exist == 1)
			  {
				++conflict;
				if (compList[i].dist<compList[j].dist)
				  {
				  compList.erase(compList.begin()+j);
				  }
				else
				  {
				  compList.erase(compList.begin()+i);
				  }
				//myText+="\nConflict resolved.\n";
				j=i;
			  }
			else
		  	{
				i++;
				j=i;
		  	}
	  	}
		myText+="\tConflicts resolved:" + QString::number(conflict);
		myText+= "\nTrace List size:\t" + QString::number(traceList.size());
		myText+="\tNumber of computed distances:\t" + QString::number(compList.size());
		for (i=0;i<compList.size(); i++)
		{			
			if (compList[i].dist<= gapMax && ((fabs(compList[i].angle) < .9))||(fabs(compList[i].angle) >= 2.2))	//
		  	{
				dtext+= "\nTrace " + QString::number(compList[i].Trace1->GetId());
				dtext+= " compared to trace "+ QString::number(compList[i].Trace2->GetId() );
				dtext+=" gap size of: " + QString::number(compList[i].dist); 
				dtext+=" endpts " + QString::number(compList[i].endPT1); 
				dtext+=" and " + QString::number(compList[i].endPT2);
				dtext+=" angle of " + QString::number(compList[i].angle);
				tobj->mergeTraces(compList[i].endPT1,compList[i].endPT2);
				++mergeCount;
		  	}	//end of if
			else
		  	{
				this->HighlightSelected(compList[i].Trace1, .125);
				this->HighlightSelected(compList[i].Trace2, .125);
				grayList.push_back( compList[i]);
				grayText+="\nAngle of: " + QString::number(compList[i].angle);
				grayText+="\tWith a distance of: " + QString::number(compList[i].dist);
				
			 } //end of else
		}//end of for merge
		myText+="\nNumber of merged lines:\t" + QString::number(mergeCount);
		this->Rerender();
		if (grayList.size()>=1)
		{
			myText+="\nNumber of further possible lines:\t" + QString::number(grayList.size());
			mergeAll = MergeInfo->addButton("Merge All", QMessageBox::YesRole);
			mergeNone = MergeInfo->addButton("Merge None", QMessageBox::NoRole);
			MergeInfo->setDetailedText(grayText);
		}		//end if graylist size
		else
		{
			MergeInfo->setDetailedText(dtext);
		}		//end else
	  }

	else
	{
		myText+= "\nNo merges possible, set higher tolerances\n"; 
	}
	MergeInfo->setText(myText);	
	MergeInfo->show();
	MergeInfo->exec();
	if(MergeInfo->clickedButton()==mergeAll)
	{
		for (j=0; j<grayList.size();j++)
		{
			tobj->mergeTraces(grayList[j].endPT1,grayList[j].endPT2);
		}
	}
	else if(MergeInfo->clickedButton()==mergeNone)
	{
		grayList.clear();
	}
}


void View3D::HighlightSelected(TraceLine* tline, double color)
{
	TraceLine::TraceBitsType::iterator iter = tline->GetTraceBitIteratorBegin();
	TraceLine::TraceBitsType::iterator iterend = tline->GetTraceBitIteratorEnd();

  while(iter!=iterend)
  {
	  //poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,1/t);
	  poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,color);
	  ++iter;
  }
	

}

void View3D::AddContourThresholdSliders()
{
  vtkSliderRepresentation2D *sliderRep2 = vtkSliderRepresentation2D::New();
  sliderRep2->SetValue(0.8);
  sliderRep2->SetTitleText("Threshold");
  sliderRep2->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint1Coordinate()->SetValue(0.2,0.8);
  sliderRep2->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint2Coordinate()->SetValue(0.8,0.8);
  sliderRep2->SetSliderLength(0.02);
  sliderRep2->SetSliderWidth(0.03);
  sliderRep2->SetEndCapLength(0.01);
  sliderRep2->SetEndCapWidth(0.03);
  sliderRep2->SetTubeWidth(0.005);
  sliderRep2->SetMinimumValue(0.0);
  sliderRep2->SetMaximumValue(1.0);

  vtkSliderWidget *sliderWidget2 = vtkSliderWidget::New();
  sliderWidget2->SetInteractor(Interactor);
  sliderWidget2->SetRepresentation(sliderRep2);
  sliderWidget2->SetAnimationModeToAnimate();

  vtkSlider2DCallbackContourThreshold *callback_contour = vtkSlider2DCallbackContourThreshold::New();
  callback_contour->cfilter = this->ContourFilter;
  sliderWidget2->AddObserver(vtkCommand::InteractionEvent,callback_contour);
  sliderWidget2->EnabledOn();

}

void View3D::AddPlaybackWidget(char *filename)
{

  vtkSubRep *playbackrep = vtkSubRep::New();
  playbackrep->slice_counter=0;
  playbackrep->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
  playbackrep->GetPositionCoordinate()->SetValue(0.2,0.1);
  playbackrep->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  playbackrep->GetPosition2Coordinate()->SetValue(0.8,0.1);
  

  vtkSmartPointer<vtkPlaybackWidget> pbwidget = vtkSmartPointer<vtkPlaybackWidget>::New();
  pbwidget->SetRepresentation(playbackrep);
  pbwidget->SetInteractor(Interactor);

  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer imreader = ReaderType::New();

  imreader->SetFileName(filename);
  imreader->Update();

  
  ImageType::SizeType size = imreader->GetOutput()->GetLargestPossibleRegion().GetSize();

  std::vector<vtkSmartPointer<vtkImageData> > &vtkimarray = playbackrep->im_pointer;
  vtkimarray.reserve(size[2]-1);
  printf("About to create vtkimarray contents in a loop\n");
  for(unsigned int counter=0; counter<size[2]-1; counter++)
  {
    vtkimarray.push_back(vtkSmartPointer<vtkImageData>::New());
    vtkimarray[counter]->SetScalarTypeToUnsignedChar();
    vtkimarray[counter]->SetDimensions(size[0],size[1],2);
    vtkimarray[counter]->SetNumberOfScalarComponents(1);
    vtkimarray[counter]->AllocateScalars();
    vtkimarray[counter]->SetSpacing(1/2.776,1/2.776,1);
    memcpy(vtkimarray[counter]->GetScalarPointer(),imreader->GetOutput()->GetBufferPointer()+counter*size[0]*size[1]*sizeof(unsigned char),size[0]*size[1]*2*sizeof(unsigned char));
  }

  printf("finished memcopy in playback widget\n");
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint(2,0.0);
  opacityTransferFunction->AddPoint(20,0.2);

  vtkColorTransferFunction *colorTransferFunction = vtkColorTransferFunction::New();
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(20.0,1.0,1.0,1.0);

  
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->SetInterpolationTypeToLinear();

  vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D> volumeMapper = vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D>::New();
  volumeMapper->SetSampleDistance(0.5);
  volumeMapper->SetInput(vtkimarray[playbackrep->slice_counter]);
  
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
  volume->SetPickable(0);
  Renderer->AddVolume(volume);
  printf("Added volume in playback widget");
  this->Volume = volume;
  playbackrep->vmapper=volumeMapper;
  /*playbackrep->im_pointer = &vtkimarray;*/
  playbackrep->Print(std::cout);
  
  this->QVTK->GetRenderWindow()->Render();
}
void View3D::AddVolumeSliders()
{
  vtkSliderRepresentation2D *sliderRep = vtkSliderRepresentation2D::New();
  sliderRep->SetValue(0.1);
  sliderRep->SetTitleText("Opacity");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.2,0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.8,0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.01);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);
  sliderRep->SetMinimumValue(0.0);
  sliderRep->SetMaximumValue(1.0);

  vtkSliderWidget *sliderWidget = vtkSliderWidget::New();
  sliderWidget->SetInteractor(Interactor);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetAnimationModeToAnimate();

  vtkSlider2DCallbackBrightness *callback_brightness = vtkSlider2DCallbackBrightness::New();
  callback_brightness->volume = this->Volume;
  sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback_brightness);
  sliderWidget->EnabledOn();
  

// slider 2

  vtkSliderRepresentation2D *sliderRep2 = vtkSliderRepresentation2D::New();
  sliderRep2->SetValue(0.8);
  sliderRep2->SetTitleText("Brightness");
  sliderRep2->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint1Coordinate()->SetValue(0.2,0.9);
  sliderRep2->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint2Coordinate()->SetValue(0.8,0.9);
  sliderRep2->SetSliderLength(0.02);
  sliderRep2->SetSliderWidth(0.03);
  sliderRep2->SetEndCapLength(0.01);
  sliderRep2->SetEndCapWidth(0.03);
  sliderRep2->SetTubeWidth(0.005);
  sliderRep2->SetMinimumValue(0.0);
  sliderRep2->SetMaximumValue(1.0);

  vtkSliderWidget *sliderWidget2 = vtkSliderWidget::New();
  sliderWidget2->SetInteractor(Interactor);
  sliderWidget2->SetRepresentation(sliderRep2);
  sliderWidget2->SetAnimationModeToAnimate();

  vtkSlider2DCallbackContrast *callback_contrast = vtkSlider2DCallbackContrast::New();
  callback_contrast->volume = this->Volume;
  sliderWidget2->AddObserver(vtkCommand::InteractionEvent,callback_contrast);
  sliderWidget2->EnabledOn();

}
void View3D::readImg(char* sourceFile)
{ /*vtkImageReader2 * readMe = vtkImageReader2::New();
  readMe->SetDataOrigin(0.0,0.0,0.0);
  readMe->SetDataExtent(0,255,0,255,0,255);
  readMe->SetDataSpacing(1,1,1);
  readMe->SetDataByteOrderToLittleEndian();
  readMe->SetFileName(sourceFile);
  std::cout<< sourceFile << '\n';
  readMe->Update();*/
  
  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( sourceFile );
  try
  {
    reader->Update();
    }
  catch( itk::ExceptionObject & exp )
    {
    std::cerr << "Exception thrown while reading the input file " << std::endl;
    std::cerr << exp << std::endl;
    //return EXIT_FAILURE;
    }
  std::cout << "Image Read " << std::endl;
//itk-vtk connector
  typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
  ConnectorType::Pointer connector= ConnectorType::New();
  connector->SetInput( reader->GetOutput() );

  
  this->ContourFilter = vtkSmartPointer<vtkContourFilter>::New();
  this->ContourFilter->SetInput(connector->GetOutput());
  this->ContourFilter->SetValue(0,10);            // this affects render
  //this->ContourFilter = vtkSmartPointer<vtkContourFilter>::New();
  //this->ContourFilter = contour;


  
  //vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  //normals->SetInput(this->ContourFilter->GetOutput());
  //normals->SetFeatureAngle(15);

  VolumeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  VolumeMapper->SetInput(this->ContourFilter->GetOutput());
  
  VolumeActor = vtkSmartPointer<vtkActor>::New();
  VolumeActor->SetMapper(VolumeMapper);
  //VolumeAct->GetProperty()->SetOpacity(.3);
  VolumeActor->GetProperty()->SetRepresentationToWireframe();
  VolumeActor->GetProperty()->SetColor(0.5,0.5,0.5);
  VolumeActor->SetPickable(0);
  //VolumeActor->SetScale(1/2.776,1/2.776,1);
  
  Renderer->AddActor(VolumeActor);
  this->QVTK->GetRenderWindow()->Render();
  std::cout << "contour rendered \n";
}

void View3D::rayCast(char *raySource)
{
  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer i2spReader = ReaderType::New();
  i2spReader->SetFileName( raySource );
  try
  {
    i2spReader->Update();
    }
  catch( itk::ExceptionObject & exp )
    {
    std::cerr << "Exception thrown while reading the input file " << std::endl;
    std::cerr << exp << std::endl;
    //return EXIT_FAILURE;
    }
  std::cout << "Image Read " << std::endl;
//itk-vtk connector
  typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
  ConnectorType::Pointer connector= ConnectorType::New();
  connector->SetInput( i2spReader->GetOutput() );
  vtkImageToStructuredPoints *i2sp = vtkImageToStructuredPoints::New();
  i2sp->SetInput(connector->GetOutput());


  
  ImageType::SizeType size = i2spReader->GetOutput()->GetLargestPossibleRegion().GetSize();
  vtkSmartPointer<vtkImageData> vtkim = vtkSmartPointer<vtkImageData>::New();
  vtkim->SetScalarTypeToUnsignedChar();
  vtkim->SetDimensions(size[0],size[1],size[2]);
  vtkim->SetNumberOfScalarComponents(1);
  vtkim->AllocateScalars();

  memcpy(vtkim->GetScalarPointer(),i2spReader->GetOutput()->GetBufferPointer(),size[0]*size[1]*size[2]*sizeof(unsigned char));

// Create transfer mapping scalar value to opacity
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();

  opacityTransferFunction->AddPoint(2,0.0);
  opacityTransferFunction->AddPoint(50,0.1);
 // opacityTransferFunction->AddPoint(40,0.1);
  // Create transfer mapping scalar value to color
  // Play around with the values in the following lines to better vizualize data
  vtkColorTransferFunction *colorTransferFunction = vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(50.0,0.5,0.5,0);

  // The property describes how the data will look
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
  //  volumeProperty->ShadeOn();
    volumeProperty->SetInterpolationTypeToLinear();

  vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D> volumeMapper = vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D>::New();
  volumeMapper->SetSampleDistance(0.75);
  volumeMapper->SetInput(vtkim);

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  vtkVolume *volume = vtkVolume::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
  volume->SetPickable(0);
//  Renderer->AddVolume(volume);
  this->Volume = volume;
 // this->QVTK->GetRenderWindow()->Render();
  std::cout << "RayCast generated \n";
}

void View3D::closeEvent(QCloseEvent *event)
{
  event->accept();
}
