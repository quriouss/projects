clear all;

% Load data
rootfolder = fullfile('101_ObjectCategories');

category = {'airplanes', 'ferry', 'laptop'};

imds = imageDatastore(fullfile(rootfolder, category),'LabelSource', 'foldernames');
tbl = countEachLabel(imds);

[imdsTrain, imdsValidation] = splitEachLabel(imds, 0.7, 'randomized');

% Use of pre-trained network - Alexnet
net = alexnet; 

inputSize = net.Layers(1).InputSize;

layersTransfer = net.Layers(1:end-3);

numClasses = numel(categories(imdsTrain.Labels));

layers = [
    layersTransfer
    fullyConnectedLayer(numClasses,'WeightLearnRateFactor',20,'BiasLearnRateFactor',20)
    softmaxLayer
    classificationLayer];

% Training
pixelRange = [-30 30];
imageAugmenter = imageDataAugmenter( ...
    'RandXReflection',true, ...
    'RandXTranslation',pixelRange, ...
    'RandYTranslation',pixelRange);

augimdsTrain = augmentedImageDatastore(inputSize, ...
    imdsTrain, 'ColorPreprocessing','gray2rgb','DataAugmentation',imageAugmenter);
augimdsValidation =  augmentedImageDatastore(inputSize, ...
    imdsValidation, 'ColorPreprocessing','gray2rgb','DataAugmentation',imageAugmenter);


options = trainingOptions('sgdm', ...
    'MiniBatchSize',10, ...
    'MaxEpochs',6, ...
    'InitialLearnRate',1e-4, ...
    'Shuffle','every-epoch', ...
    'ValidationData',augimdsValidation, ...
    'ValidationFrequency',3, ...
    'Verbose',false, ...
    'Plots','training-progress');

netTransfer = trainNetwork(augimdsTrain, layers, options);

% Validation
[YPred, scores] = classify(netTransfer, augimdsValidation);

idx = randperm(numel(imdsValidation.Files), 4);

figure;
for i = 1:4
    subplot(2,2,i)
    I = readimage(imdsValidation,idx(i));
    imshow(I)
    label = YPred(idx(i));
    title(string(label));
end

% Accuracy
accuracy = mean(YPred == imdsValidation.Labels);

% Loss
Mdl = fitcecoc(grp2idx(YPred), grp2idx(imdsValidation.Labels));
training_error = loss(Mdl, grp2idx(YPred), grp2idx(imdsValidation.Labels));

% Confusion matrix
figure;
plotconfusion(imdsValidation.Labels, YPred)
