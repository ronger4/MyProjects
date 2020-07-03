
n = input('Choose a Group: ');

switch n
    case 0
        im = imread('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.15 11-40-54.jpg');
        [path, filename,~] = fileparts('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.12 08-05-01.jpg')
    case 3
        im = imread('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.15 11-40-54.jpg');
        [path, filename,~] = fileparts('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.12 08-05-01.jpg')
    case 4
        im = imread('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.30 08-38-48.jpg');
        [path, filename,~] = fileparts('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.30 08-38-48.jpg')
    case 2
         im = imread('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.11 12-16-06.jpg');
        [path, filename,~] = fileparts('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.11 12-16-06.jpg')
    case 1
        im = imread('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.07 09-45-08.jpg');
        [path, filename,~] = fileparts('C:\Users\Ron\Desktop\ProjectDemo\T2019.11.07 09-45-08.jpg')
end



figure(1);
imshow(im);
title('Original Image')

%%Get Address and Time
addr = sscanf(filename,'T%d.%d.%d %d-%d-%d');

year = addr(1);
month = addr(2);
day = addr(3);
hour = addr(4);
min = addr(5);
sec = addr(6);

%%
%################Crop image##############################3

I2 = imcrop(im,[241, 25, 1435, 950]); %for regular
figure(2);
imshow(I2);
title('After Crop')










%K-means:
lab_I2 = rgb2lab(I2);
ab = lab_I2(:,:,1:3);
ab = im2single(ab);
nColors = 3;
% repeat the clustering 3 times to avoid local minima
L = imsegkmeans(ab,nColors,'NumAttempts',3);
B = (L == 2);

mask  =  (B);
maskedRgbImage = bsxfun(@times, I2, cast(mask, 'like', I2));



greenChannela = maskedRgbImage(:, :, 2);


%check if we need to revert the label mask.
if(mean2(greenChannela)>50)
    B = (L == 1);
    mask  =  (B );
    maskedRgbImage = bsxfun(@times, I2, cast(mask, 'like', I2));
end

figure(3);
imshow(maskedRgbImage);
title('After Segmentation')


%%

% Extract the individual red, green, and blue color channels.
%#################Feature Extraction###########################
redChannel = maskedRgbImage(:, :, 1);
greenChannel = maskedRgbImage(:, :, 2);
blueChannel = maskedRgbImage(:, :, 3);
greenMean = mean2(greenChannel);
redMean = mean2(redChannel);
blueMean = mean2(blueChannel);

sumer = (greenChannel - redChannel);
sumeb = (greenChannel - blueChannel);


PDer = mean2(sumer);
PDeb = mean2(sumeb);

% Get the dimensions of the image.
[rows, columns, numberOfColorChannels] = size(maskedRgbImage);
PDermat = ones(rows,columns) .* PDer;
PDebmat = ones(rows,columns) .* PDeb;


sumvr = (cast(greenChannel,'double') - cast(redChannel,'double') - PDermat).^2;
sumvb = (cast(greenChannel,'double') - cast(blueChannel,'double') - PDebmat).^2;
  
PDvr = mean2(sumvr);
PDvb = mean2(sumvb);

grey =rgb2gray(maskedRgbImage);
J = imresize(grey,[1000 1000]);
figure(4)
imshow(J)
[a1,h1,v1,d1] = haart2(J,1);
[a2,h2,v2,d2] = haart2(J,2);


figure(5)
subplot(2,2,1)
maximum = max(a1);
A = a1 ./ maximum;
imshow(A)
title('Level 1 Low Freq Component');

subplot(2,2,2)
maximum = max(h1);
A = h1 ./ maximum;
imshow(A)
title('Level 1 Horizontal');

subplot(2,2,3)
maximum = max(v1);
A = v1 ./ maximum;
imshow(A)
title('Level 1 Vertical');

subplot(2,2,4)
maximum = max(d1);
A = d1 ./ maximum;
imshow(A)
title('Level 1 Diagonal');


figure(8)
subplot(2,2,1)
maximum = max(a2);
A = a2 ./ maximum;
imshow(A)
title('Level 2 Low Freq Component');

subplot(2,2,2)
maximum = max(h2{1,2});
A = h2{1,2} ./ maximum;
imshow(A)
title('Level 2 Horizontal');

subplot(2,2,3)
maximum = max(v2{1,2});
A = v2{1,2} ./ maximum;
imshow(A)
title('Level 2 Vertical');

subplot(2,2,4)
maximum = max(d2{1,2});
A = d2{1,2} ./ maximum;
imshow(A)
title('Level 2 Diagonal');



h1 = h1.*h1;
a1 = a1.*a1;
v1 = v1.*v1;
d1 = d1.*d1;
a2 = a2.*a2;
h2{1,2} = h2{1,2}.*h2{1,2};
v2{1,2} = v2{1,2}.*v2{1,2};
d2{1,2} = d2{1,2}.*d2{1,2};

FehOne = mean2(h1) ./ (mean2(a1)+mean2(h1)+mean2(v1)+mean2(d1));
FevOne = mean2(v1) ./ (mean2(a1)+mean2(h1)+mean2(v1)+mean2(d1));
FedOne = mean2(d1) ./ (mean2(a1)+mean2(h1)+mean2(v1)+mean2(d1));
FeaTwo = mean2(a2) ./ (mean2(a2)+mean2(h2{1,2})+mean2(v2{1,2})+mean2(d2{1,2}));
FehTwo = mean2(h2{1,2}) ./ (mean2(a2)+mean2(h2{1,2})+mean2(v2{1,2})+mean2(d2{1,2}));
FevTwo = mean2(v2{1,2}) ./ (mean2(a2)+mean2(h2{1,2})+mean2(v2{1,2})+mean2(d2{1,2}));
FedTwo = mean2(d2{1,2}) ./ (mean2(a2)+mean2(h2{1,2})+mean2(v2{1,2})+mean2(d2{1,2}));
%%

%###################Create Dataset#############################

T = readtable('C:\Users\Ron\Desktop\ProjectDemo\datasetDemo.csv');
newtable = table();
newtable.greenMean = greenMean;
newtable.redMean = redMean;
newtable.blueMean = blueMean;
newtable.PDer = PDer;
newtable.PDeb = PDeb;
newtable.PDvr = PDvr;
newtable.PDvb = PDvb;
newtable.FehOne = FehOne;
newtable.FevOne = FevOne;
newtable.FedOne = FedOne;
newtable.FeaTwo = FeaTwo;
newtable.FehTwo = FehTwo;
newtable.FevTwo = FevTwo;
newtable.FedTwo = FedTwo;
newtable.month = month;
newtable.day = day;
newtable.hour = hour;
newtable.min = min;
newtable.sec = sec;
newtable.group = 0;

T = [T;newtable];
writetable(T,'C:\Users\Ron\Desktop\ProjectDemo\datasetDemo.csv');




%--------------------------------------------------------------------------
