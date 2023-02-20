clear all;

% Load data
image_folder = 'FIRE\Images';     % Upload images with full path
filenames = dir(fullfile(image_folder, '*.jpg'));                         % Read all images with specified extention
total_images = numel(filenames);                                          % Count total number of images present in that folder
 
full_name = fullfile(image_folder, filenames(19).name);                   % It will specify images names with full path and extension
image_1 = imread(full_name);                                              % Read images  

% Show first image
figure (1);                           
imshow(image_1);  
title('Reference image')

full_name_2 = fullfile(image_folder, filenames(20).name);                 % It will specify images names with full path and extension
image_2 = imread(full_name_2);                                            % Read images 

% Show second image
figure (2);                          
imshow(image_2);  
title('Test Image')

fileID = fopen('FIRE\Ground Truth\control_points_A10_1_2.txt','r');       % Read the points from the Ground Truth files

formatSpec = '%f %f';
size_points = [4 10];

points = fscanf(fileID, formatSpec, size_points);
points  = points';

A = points(:, [1:2]);

b1 = points(:, 3);
b2 = points(:, 4);

R = ones(10, 1);
A = [A R];

p = A\b1;
q = A\b2;

affine = [p'; q'; 0 0 1];

affine = affine';

T = affine2d(affine);
transformed_image = imwarp(image_1, T, 'OutputView', imref2d( size(image_1) ));

% Show the image after the affine transformation
figure;
imshow(transformed_image)
title('Reference image after transformation')

% Optimization
figure;
imshowpair(image_2, image_1,'blend','Scaling','joint')
title('Blended overlay of reference and test images')

figure;
imshowpair(image_2, transformed_image,'blend','Scaling','joint')
title('Blended overlay of transformed and test image')

% RGB to Grey
image_1 = rgb2gray(image_1);
image_2 = rgb2gray(image_2);
transformed_image = rgb2gray(transformed_image);

% Find the edges (vessels)
edged_image = edge(image_1,'canny', [0.1 0.15], 12);

[i,j] =  find(edged_image > 0);

% Projection
figure;
imshow(image_2);
title('Edges of reference image on test image')
hold On
plot(j,i,'.');

id1 = find(image_1 > 0 & image_2 > 0);
id2 = find(transformed_image > 0 & image_2 > 0);

% Correlation coefficient
corr_before = corr2(image_1(id1),image_2(id1))
corr_after = corr2(transformed_image(id2), image_2(id2))

% Mutual Information
mi_before = mi(image_1(id1), image_2(id1))
mi_after = mi(transformed_image(id2), image_2(id2))