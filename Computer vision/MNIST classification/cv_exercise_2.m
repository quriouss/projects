clear all;

% Load data
images = loadMNISTImages('train-images.idx3-ubyte');
l = loadMNISTLabels('train-labels.idx1-ubyte');

% Cross varidation (train: 70%, test: 30%)
cv = cvpartition(size(images, 2),'HoldOut', 0.3);
idx = test(cv);

% Separate to training and test data
dataTrain = images(:, ~idx);
dataTest = images(:, idx);

labelTrain = l(~idx, :);
labelTest = l(idx, :);

labelTrain = labelTrain';
labelTest = labelTest';

labelTest(labelTest==0) = 10;       % dummyvar function doesn't take zeroes
labelTest = dummyvar(labelTest)';

labelTrain(labelTrain==0) = 10;
labelTrain = dummyvar(labelTrain)';

% Use scaled conjugate gradient for training
trainFcn = 'trainscg';

% Choose a Performance Function
performFcn = 'crossentropy';        % Cross-Entropy

% Build neural network
net = patternnet([100 100], trainFcn, performFcn);

% Setup Division of Data for Training, Validation, Testing
net.divideParam.trainRatio = 80/100;
net.divideParam.valRatio = 20/100;
net.divideParam.testRatio = 0/100;

% Train the Network
[net,tr] = train(net, dataTrain, labelTrain);

% Test the Network with MNIST test data
output = net(dataTest);
performance_testdata = perform(net, labelTest, output);
labelTest_vector = vec2ind(labelTest);
output_vector = vec2ind(output);
percentErrors_testdata = sum(labelTest_vector ~=  output_vector)/numel(labelTest_vector);

% Confusion matrix
figure;
plotconfusion(labelTest, output)