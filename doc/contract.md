Description:
============
Our goal is to develop a web-based interface for an active-learning machine learning system. The idea of the active 
learning system is to present users with a series of examples and ask them to label these examples by their class. 
This information is used in the background to train a classifier that is refined through iterations of feedback, 
hopefully converging to a decent accuracy. This system enables natural interaction between users and machine learning 
algorithms.

The application we will investigate is the classification of cells in whole-slide histology images. The system consists 
of three main pages: Main view, slide view and grid view.

In the main view, users will be able to create a new classifier and assign it a name, add classes to this classifier 
and assign colors to the classes. They will also be able to export the training data table and see a list of previously 
trained classifiers.

In the slide view, users can interact with the data in the whole-slide format. They can click on cells to add them to 
the training set (for initialization) and view the results of the currently trained classifier by color coding of the 
boundaries.

In the grid view, users are presented with the computer-selected training examples displayed in a grid. The users can 
click on this grid to correct errors (highlighting misclassifications as red) and points they are uncertain of 
(highlight as gray). Basic information regarding the current iteration, number of examples in each class, and accuracy 
are displayed.

Roles & Responsibilities:
==========================
The software developer will only be responsible for the user interface elements. All data and databases that populate these elements will be provided by Dr. Cooper’s research lab. Emory will be responsible for establishing the slide viewer and boundary display capability (already complete), the database containing boundary values, training set databases, all machine learning and any information that populates the webpage.
Project code will be stored in a repository accessible to all members (GitHub).

Budget & Timeline:
==================
The project is broken up into two phases: the prototyping phase that will establish basic and essential functions and 
the cleanup phase that will refine the appearance of the prototype and add additional non-essential functions.

### Phase I – Prototype & Proof of Concept

#### Main view:
* Naming classifier and establishing classes for a new classifier
	* Assigning colors to classes
* Deleting an existing classifier from the system	
* Exporting training data table to text
#### Viewer mode:
* Select examples to initialize classifier
	* Pull-down to select from classes
	* Recording clicks and inserting cell and class info into table
* Color coding boundaries by class
	* Cells boundaries are displayed as an SVG – pull cell classes from machine learning database and set their SVG color code according to class
#### Grid view:
* Initial grid layout – hardcoded layout (# rows/columns), form IIP queries to pull image data into grid
		* Clicking and color highlighting for review
* Button to initiate training
	* Basic feedback on accuracy – table in margin
      * Iteration number
      * Items in each class
      * Accuracy

### Phase II  - Cleanup and Add Functionality
Improve overall appearance – add header graphics and cleanup layout
#### Main view:
* Selecting datasets
* Parameters for machine learning 
	* Drop down to select classifier type	
	* Text boxes to capture algorithmic parameters - customized by learning algorithm selection
#### Viewer mode:
* Display “information” hover/click data for cells (certainty measure – a single scalar)
#### Grid view:
* Deep linking of training samples – double click on cell in grid sends you to that example on slide and highlights the example
* Visualization – line plot of accuracy vs. # samples, histogram of uncertainty measures
* Adjustable grid dimensions
